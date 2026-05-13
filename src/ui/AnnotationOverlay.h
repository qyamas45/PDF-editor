#pragma once
#include <QWidget>
#include <QPainterPath>
#include <QVector>
#include <QPointF>
#include <QRectF>
#include <QString>
#include "tools/ToolItem.h"

class QAbstractScrollArea;
class QLineEdit;

struct TextAnnotation {
    QPointF position;
    QString text;
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

    const QVector<QPainterPath>&   strokes()          const { return m_strokes; }
    const QVector<TextAnnotation>& textAnnotations()   const { return m_textAnnotations; }

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
    QVector<QPainterPath>    m_strokes;
    bool                     m_drawing = false;

    QVector<TextAnnotation>  m_textAnnotations;
    QLineEdit               *m_textInput = nullptr;

    QAbstractScrollArea     *m_scrollArea;

    SelectedItem             m_selection;
    QPointF                  m_dragLastPos;
    bool                     m_dragging = false;

    QPointF        scrollOffset() const;
    void           commitTextInput();
    void           updateMousePassthrough();
    SelectedItem   hitTestAt(QPointF pos) const;
    QRectF         selectionRect() const;
};
