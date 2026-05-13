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

    // Committed strokes (stored in document space)
    QPen strokePen(QColor(220, 50, 50, 200), 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(strokePen);
    p.setBrush(Qt::NoBrush);
    for (const QPainterPath &path : m_strokes)
        p.drawPath(path);

    // In-progress stroke
    if (m_drawing)
        p.drawPath(m_currentStroke);

    // Text annotations (stored in document space)
    QFont font;
    font.setFamily("Arial");
    font.setPixelSize(16);
    p.setFont(font);
    p.setPen(QColor(30, 30, 180, 220));
    for (const TextAnnotation &ann : m_textAnnotations)
        p.drawText(ann.position, ann.text);

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

    } else if (m_activeTool == ToolType::Text) {
        if (m_textInput && m_textInput->isVisible()) {
            commitTextInput();
            return;
        }

        m_textInput = new QLineEdit(this);
        m_textInput->setFrame(false);
        m_textInput->setStyleSheet(
            "QLineEdit {"
            "  background: rgba(255,255,220,200);"
            "  border: 1px dashed #4A90E2;"
            "  border-radius: 2px;"
            "  color: #1a1a9a;"
            "  font-size: 16px;"
            "  padding: 1px 3px;"
            "}"
        );
        m_textInput->setMinimumWidth(120);
        // QLineEdit is a child of the overlay, so place it in viewport coords
        m_textInput->move(event->position().toPoint());
        m_textInput->show();
        m_textInput->setFocus();

        // Capture document position for the stored annotation
        connect(m_textInput, &QLineEdit::returnPressed, this, [this, docPos]() {
            if (!m_textInput->text().isEmpty()) {
                QPointF textPos(docPos.x() + 3, docPos.y() + 18);
                m_textAnnotations.append({ textPos, m_textInput->text() });
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

        if (m_selection.kind == SelectionKind::Stroke)
            m_strokes[m_selection.index].translate(delta.x(), delta.y());
        else if (m_selection.kind == SelectionKind::Text)
            m_textAnnotations[m_selection.index].position += delta;

        update();
    } else if (m_activeTool == ToolType::Draw && m_drawing) {
        m_currentStroke.lineTo(docPos);
        update();
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

    if (m_activeTool == ToolType::Draw && m_drawing) {
        QPointF docPos = event->position() + scrollOffset();
        m_currentStroke.lineTo(docPos);
        m_strokes.append(m_currentStroke);
        m_currentStroke = QPainterPath();
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
        m_textAnnotations.append({ pos, m_textInput->text() });
    m_textInput->hide();
    m_textInput->deleteLater();
    m_textInput = nullptr;
    update();
}

SelectedItem AnnotationOverlay::hitTestAt(QPointF pos) const
{
    QFont font;
    font.setFamily("Arial");
    font.setPixelSize(16);
    QFontMetricsF fm(font);

    for (int i = m_textAnnotations.size() - 1; i >= 0; --i) {
        const TextAnnotation &ann = m_textAnnotations[i];
        qreal w = fm.horizontalAdvance(ann.text);
        QRectF rect(ann.position.x() - 3, ann.position.y() - fm.ascent() - 2,
                    w + 6, fm.height() + 4);
        if (rect.contains(pos))
            return { SelectionKind::Text, i };
    }

    QPainterPathStroker stroker;
    stroker.setWidth(10.0);
    for (int i = m_strokes.size() - 1; i >= 0; --i) {
        if (stroker.createStroke(m_strokes[i]).contains(pos))
            return { SelectionKind::Stroke, i };
    }

    return {};
}

QRectF AnnotationOverlay::selectionRect() const
{
    if (m_selection.kind == SelectionKind::Stroke && m_selection.index >= 0)
        return m_strokes[m_selection.index].boundingRect().adjusted(-5, -5, 5, 5);

    if (m_selection.kind == SelectionKind::Text && m_selection.index >= 0) {
        const TextAnnotation &ann = m_textAnnotations[m_selection.index];
        QFont font;
        font.setFamily("Arial");
        font.setPixelSize(16);
        QFontMetricsF fm(font);
        qreal w = fm.horizontalAdvance(ann.text);
        return QRectF(ann.position.x() - 3, ann.position.y() - fm.ascent() - 2,
                      w + 6, fm.height() + 4).adjusted(-4, -4, 4, 4);
    }

    return {};
}
