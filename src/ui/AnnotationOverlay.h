#pragma once
#include <QWidget>
#include <QPainterPath>
#include <QVector>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QColor>
#include "tools/ToolItem.h"

class QAbstractScrollArea;
class QLineEdit;

struct TextAnnotation {
    QPointF position;
    QString text;
    QColor  color    = QColor(30, 30, 180, 220);
    int     fontSize = 16;
};

struct Stroke {
    QPainterPath path;
    QVector<QPointF> points;
    QColor       color = QColor(220, 50, 50, 200);
    qreal        width = 2.5;
};

enum class SelectionKind { None, Stroke, Text };

struct SelectedItem {
    SelectionKind kind  = SelectionKind::None;
    int           index = -1;
};

class AnnotationOverlay : public QWidget {
    Q_OBJECT

public:
    explicit AnnotationOverlay(QAbstractScrollArea *scrollArea);

public slots:
    void setActiveTool(ToolType type);
    void setStrokeColor(QColor color);
    void setStrokeWidth(qreal width);
    void setTextColor(QColor color);
    void setFontSize(int size);
    void setEraserRadius(qreal radius);

    const QVector<Stroke>&         strokes()          const { return m_strokes; }
    const QVector<TextAnnotation>& textAnnotations()   const { return m_textAnnotations; }

    // Drop every annotation without emitting annotationsChanged(); used when a
    // new document is loaded and the dirty state is being reset anyway.
    void clearAnnotations();

signals:
    // Emitted whenever the stored annotations are added to, removed, or moved.
    // MainWindow uses this to drive the enabled state of File > Save.
    void annotationsChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    ToolType                 m_activeTool = ToolType::None;

    QPainterPath             m_currentStroke;
    QVector<Stroke>          m_strokes;
    QVector<QPointF>         m_currentPoints;
    QVector<bool>            m_eraseClassifyBuf;   // reused across eraseFragmentAt calls
    qreal                    m_eraserRadius = 8.0;
    bool                     m_drawing  = false;
    bool                     m_erasing  = false;

    QVector<TextAnnotation>  m_textAnnotations;
    QLineEdit               *m_textInput = nullptr;

    QAbstractScrollArea     *m_scrollArea;

    SelectedItem             m_selection;
    QPointF                  m_dragLastPos;
    bool                     m_dragging  = false;
    bool                     m_dragMoved = false;   // true once a drag actually shifted an item

    QPointF        scrollOffset() const;
    void           commitTextInput();
    void           updateMousePassthrough();
    SelectedItem   hitTestAt(QPointF pos) const;
    QRectF         selectionRect() const;
    bool           eraseFragmentAt(QPointF center, qreal radius);
    
    QColor m_strokeColor  = QColor(220, 50, 50, 200);
    qreal  m_strokeWidth  = 2.5;
    QColor m_textColor    = QColor(30, 30, 180, 220);
    int    m_fontSize     = 16;
};
