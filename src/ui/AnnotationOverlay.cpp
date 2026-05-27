#include "AnnotationOverlay.h"

#include <QAbstractScrollArea>
#include <QScrollBar>
#include <QPainter>
#include <QPainterPathStroker>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QFont>
#include <QFontMetricsF>

AnnotationOverlay::AnnotationOverlay(QAbstractScrollArea *scrollArea)
    : QWidget(scrollArea->viewport())
    , m_scrollArea(scrollArea)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
    resize(scrollArea->viewport()->size());
    scrollArea->viewport()->installEventFilter(this);

    // Repaint whenever the user scrolls so annotations track the page
    connect(scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, [this](int) { update(); });
    connect(scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this](int) { update(); });
}

QPointF AnnotationOverlay::scrollOffset() const
{
    return QPointF(m_scrollArea->horizontalScrollBar()->value(),
                   m_scrollArea->verticalScrollBar()->value());
}

void AnnotationOverlay::setActiveTool(ToolType type)
{
    if (m_textInput && m_textInput->isVisible())
        commitTextInput();

    m_selection  = {};
    m_dragging   = false;
    m_erasing    = false;
    m_activeTool = type;
    updateMousePassthrough();
    update();
}

void AnnotationOverlay::updateMousePassthrough()
{
    setAttribute(Qt::WA_TransparentForMouseEvents, m_activeTool == ToolType::None);
}

void AnnotationOverlay::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Shift painter into document space so annotations follow the scrolled page
    QPointF off = scrollOffset();
    p.translate(-off.x(), -off.y());

    // Visible document area — used to cull off-screen annotations
    const QRectF visibleRect(off.x(), off.y(), width(), height());

    // Committed strokes — skip any whose bounding box lies outside the viewport
    p.setBrush(Qt::NoBrush);
    for (const Stroke &s : m_strokes) {
        if (!s.path.boundingRect().intersects(visibleRect))
            continue;
        p.setPen(QPen(s.color, s.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(s.path);
    }

    // In-progress stroke uses the current active settings
    if (m_drawing) {
        p.setPen(QPen(m_strokeColor, m_strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(m_currentStroke);
    }

    // Text annotations — skip off-screen ones; cache the QFont when size repeats
    int lastFontSize = -1;
    const QRectF textVisRect = visibleRect.adjusted(0, -40, 0, 10); // extra margin for ascenders
    for (const TextAnnotation &ann : m_textAnnotations) {
        if (!textVisRect.contains(ann.position))
            continue;
        if (ann.fontSize != lastFontSize) {
            QFont font("Arial");
            font.setPixelSize(ann.fontSize);
            p.setFont(font);
            lastFontSize = ann.fontSize;
        }
        p.setPen(ann.color);
        p.drawText(ann.position, ann.text);
    }

    // Selection highlight (document space)
    if (m_selection.kind != SelectionKind::None) {
        QRectF rect = selectionRect();
        if (!rect.isNull()) {
            p.setPen(QPen(QColor("#4A90E2"), 1.5, Qt::DashLine));
            p.setBrush(Qt::NoBrush);
            p.drawRect(rect);

            const qreal hs = 5;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#4A90E2"));
            for (const QPointF &c : { rect.topLeft(), rect.topRight(),
                                      rect.bottomLeft(), rect.bottomRight() }) {
                p.drawRect(QRectF(c.x() - hs / 2, c.y() - hs / 2, hs, hs));
            }
        }
    }
}
 
// The following code is for handling mouse interactions 
// with the annotation overlay, including drawing strokes, 
// placing text annotations, and selecting/moving existing annotations. 
// All positions are managed in document space (accounting for scrolling) to ensure annotations stay correctly aligned with the PDF content.
// Mouse events are processed based on the currently active tool, 
// and the overlay updates its display accordingly.
void AnnotationOverlay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    // All positions stored in document (scrolled) space
    QPointF docPos = event->position() + scrollOffset();

    if (m_activeTool == ToolType::Select) {
        m_selection   = hitTestAt(docPos);
        m_dragging    = m_selection.kind != SelectionKind::None;
        m_dragLastPos = docPos;
        if (m_selection.kind != SelectionKind::None)
            setFocus();
        update();
        return;

    } else if (m_activeTool == ToolType::Draw) {
        m_drawing = true;
        m_currentStroke = QPainterPath();
        m_currentStroke.moveTo(docPos);
        m_currentPoints.clear();
        m_currentPoints.append(docPos);
        
    } else if (m_activeTool == ToolType::Erase) {
        m_erasing = true;
        eraseFragmentAt(docPos, m_eraserRadius);
        // text has no geometry to fragment — erase whole on press only
        for (int i = m_textAnnotations.size() - 1; i >= 0; --i) {
            const TextAnnotation &ann = m_textAnnotations[i];
            QFont font;
            font.setFamily("Arial");
            font.setPixelSize(ann.fontSize);
            QFontMetricsF fm(font);
            QRectF rect(ann.position.x() - 3, ann.position.y() - fm.ascent() - 2,
                        fm.horizontalAdvance(ann.text) + 6, fm.height() + 4);
            if (rect.contains(docPos)) {
                m_textAnnotations.removeAt(i);
                break;
            }
        }
        update();
        return;

    } else if (m_activeTool == ToolType::Text) {
        if (m_textInput && m_textInput->isVisible()) {
            commitTextInput();
            return;
        }

        m_textInput = new QLineEdit(this);
        m_textInput->setFrame(false);
        m_textInput->setStyleSheet(QString(
            "QLineEdit {"
            "  background: rgba(255,255,220,200);"
            "  border: 1px dashed #4A90E2;"
            "  border-radius: 2px;"
            "  color: %1;"
            "  font-size: %2px;"
            "  padding: 1px 3px;"
            "}"
        ).arg(m_textColor.name()).arg(m_fontSize));
        m_textInput->setMinimumWidth(120);
        // QLineEdit is a child of the overlay, so place it in viewport coords
        m_textInput->move(event->position().toPoint());
        m_textInput->show();
        m_textInput->setFocus();

        // Capture document position for the stored annotation
        const QColor activeTextColor = m_textColor;
        const int    activeFontSize  = m_fontSize;
        connect(m_textInput, &QLineEdit::returnPressed, this, [this, docPos, activeTextColor, activeFontSize]() {
            if (!m_textInput->text().isEmpty()) {
                QPointF textPos(docPos.x() + 3, docPos.y() + 18);
                m_textAnnotations.append({ textPos, m_textInput->text(), activeTextColor, activeFontSize });
            }
            m_textInput->hide();
            m_textInput->deleteLater();
            m_textInput = nullptr;
            update();
        });
    }
}

void AnnotationOverlay::mouseMoveEvent(QMouseEvent *event)
{
    QPointF docPos = event->position() + scrollOffset();

    if (m_activeTool == ToolType::Select && m_dragging) {

        QPointF delta  = docPos - m_dragLastPos;
        m_dragLastPos  = docPos;

        for(QPointF &p : m_strokes[m_selection.index].points)
            p += delta;

        if (m_selection.kind == SelectionKind::Stroke)
            m_strokes[m_selection.index].path.translate(delta.x(), delta.y());
        else if (m_selection.kind == SelectionKind::Text)
            m_textAnnotations[m_selection.index].position += delta;

        update();
    } else if (m_activeTool == ToolType::Erase && m_erasing) {
        eraseFragmentAt(docPos, m_eraserRadius);
        update();
    } else if (m_activeTool == ToolType::Draw && m_drawing) {
        // Capture previous point before appending, so we can compute the dirty segment
        const QPointF prevDocPos = m_currentPoints.last();
        m_currentStroke.lineTo(docPos);
        m_currentPoints.append(docPos);

        // Only invalidate the bounding box of the new line segment (viewport coords)
        const QPointF off = scrollOffset();
        const qreal margin = m_strokeWidth / 2.0 + 2.0;
        const QRectF dirty = QRectF(prevDocPos - off, docPos - off)
                                 .normalized()
                                 .adjusted(-margin, -margin, margin, margin);
        update(dirty.toAlignedRect());
    }
}

void AnnotationOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    if (m_activeTool == ToolType::Select) {
        m_dragging = false;
        return;
    }

    if (m_activeTool == ToolType::Erase) {
        m_erasing = false;
        return;
    }

    if (m_activeTool == ToolType::Draw && m_drawing) {
        QPointF docPos = event->position() + scrollOffset();
        m_currentStroke.lineTo(docPos);
        m_currentPoints.append(docPos);
        m_strokes.append({ m_currentStroke, m_currentPoints, m_strokeColor, m_strokeWidth });
        m_currentStroke = QPainterPath();
        m_currentPoints.clear();
        m_drawing = false;
        update();
    }
}

bool AnnotationOverlay::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parent() && event->type() == QEvent::Resize)
        resize(static_cast<QResizeEvent *>(event)->size());
    return QWidget::eventFilter(watched, event);
}

void AnnotationOverlay::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete)
        && m_selection.kind != SelectionKind::None)
    {
        if (m_selection.kind == SelectionKind::Stroke)
            m_strokes.removeAt(m_selection.index);
        else if (m_selection.kind == SelectionKind::Text)
            m_textAnnotations.removeAt(m_selection.index);

        m_selection = {};
        update();
        return;
    }
    QWidget::keyPressEvent(event);
}

void AnnotationOverlay::commitTextInput()
{
    if (!m_textInput)
        return;
    // Convert the widget's viewport position back to document space
    QPointF off = scrollOffset();
    QPointF pos(m_textInput->pos().x() + 3  + off.x(),
                m_textInput->pos().y() + 18 + off.y());
    if (!m_textInput->text().isEmpty())
        m_textAnnotations.append({ pos, m_textInput->text(), m_textColor, m_fontSize });
    m_textInput->hide();
    m_textInput->deleteLater();
    m_textInput = nullptr;
    update();
}

SelectedItem AnnotationOverlay::hitTestAt(QPointF pos) const
{
    for (int i = m_textAnnotations.size() - 1; i >= 0; --i) {
        const TextAnnotation &ann = m_textAnnotations[i];
        QFont font;
        font.setFamily("Arial");
        font.setPixelSize(ann.fontSize);
        QFontMetricsF fm(font);
        qreal w = fm.horizontalAdvance(ann.text);
        QRectF rect(ann.position.x() - 3, ann.position.y() - fm.ascent() - 2,
                    w + 6, fm.height() + 4);
        if (rect.contains(pos))
            return { SelectionKind::Text, i };
    }

    QPainterPathStroker stroker;
    stroker.setWidth(10.0);
    for (int i = m_strokes.size() - 1; i >= 0; --i) {
        // Cheap bounding-box reject before the expensive createStroke expansion
        if (!m_strokes[i].path.boundingRect().adjusted(-5, -5, 5, 5).contains(pos))
            continue;
        if (stroker.createStroke(m_strokes[i].path).contains(pos))
            return { SelectionKind::Stroke, i };
    }

    return {};
}

void AnnotationOverlay::eraseFragmentAt(QPointF center, qreal radius)
{
    const qreal r2 = radius * radius;

    for (int i = m_strokes.size() - 1; i >= 0; --i) {
        // Quick bounding-box rejection before checking individual points
        if (!m_strokes[i].path.boundingRect()
                .adjusted(-radius, -radius, radius, radius)
                .contains(center))
            continue;

        const QVector<QPointF> &pts = m_strokes[i].points;

        // Classify each point using the reusable member buffer (avoids per-call heap alloc)
        m_eraseClassifyBuf.resize(pts.size());
        m_eraseClassifyBuf.fill(false);
        bool anyErased = false;
        for (int j = 0; j < pts.size(); ++j) {
            QPointF d = pts[j] - center;
            if (d.x() * d.x() + d.y() * d.y() <= r2) {
                m_eraseClassifyBuf[j] = true;
                anyErased = true;
            }
        }

        if (!anyErased)
            continue;

        // Save before removeAt invalidates the reference
        const QColor savedColor = m_strokes[i].color;
        const qreal  savedWidth = m_strokes[i].width;

        // Collect surviving sub-sequences (consecutive non-erased runs of >= 2 points)
        QVector<Stroke> replacements;
        int j = 0;
        while (j < pts.size()) {
            if (m_eraseClassifyBuf[j]) { ++j; continue; }

            Stroke sub;
            sub.color = savedColor;
            sub.width = savedWidth;
            sub.points.append(pts[j]);
            sub.path.moveTo(pts[j]);
            ++j;
            while (j < pts.size() && !m_eraseClassifyBuf[j]) {
                sub.points.append(pts[j]);
                sub.path.lineTo(pts[j]);
                ++j;
            }
            if (sub.points.size() >= 2)
                replacements.append(sub);
        }

        // Replace original stroke with surviving fragments
        m_strokes.removeAt(i);
        for (int k = 0; k < replacements.size(); ++k)
            m_strokes.insert(i + k, replacements[k]);
        // Indices below i are unaffected; the for loop --i steps past inserted entries correctly
    }
}

void AnnotationOverlay::setEraserRadius(qreal radius) { m_eraserRadius = radius; }
void AnnotationOverlay::setStrokeColor(QColor color) { m_strokeColor = color; update(); }
void AnnotationOverlay::setStrokeWidth(qreal width)  { m_strokeWidth = width; update(); }
void AnnotationOverlay::setTextColor(QColor color)   { m_textColor   = color; update(); }
void AnnotationOverlay::setFontSize(int size)        { m_fontSize    = size;  update(); }

QRectF AnnotationOverlay::selectionRect() const
{
    if (m_selection.kind == SelectionKind::Stroke && m_selection.index >= 0)
        return m_strokes[m_selection.index].path.boundingRect().adjusted(-5, -5, 5, 5);

    if (m_selection.kind == SelectionKind::Text && m_selection.index >= 0) {
        const TextAnnotation &ann = m_textAnnotations[m_selection.index];
        QFont font;
        font.setFamily("Arial");
        font.setPixelSize(ann.fontSize);
        QFontMetricsF fm(font);
        qreal w = fm.horizontalAdvance(ann.text);
        return QRectF(ann.position.x() - 3, ann.position.y() - fm.ascent() - 2,
                      w + 6, fm.height() + 4).adjusted(-4, -4, 4, 4);
    }

    return {};
}
