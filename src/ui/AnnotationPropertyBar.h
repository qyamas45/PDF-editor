#pragma once
#include <QFrame>
#include <QColor>
#include "tools/ToolItem.h"

class QVBoxLayout;
class QLabel;

class AnnotationPropertyBar : public QFrame {
    Q_OBJECT

public:
    explicit AnnotationPropertyBar(QWidget *parent = nullptr);

public slots:
    void setTool(ToolType type);

signals:
    void strokeColorChanged(QColor color);
    void strokeWidthChanged(qreal width);
    void textColorChanged(QColor color);
    void fontSizeChanged(int size);
    void eraserRadiusChanged(qreal radius);

private:
    void clearPanel();
    void buildDrawPanel();
    void buildTextPanel();
    void buildErasePanel();
    
    QVBoxLayout *m_layout;
};
