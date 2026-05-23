#pragma once
#include <iostream>
#include <QWidget>
#include <QFrame>
#include <QVector>
#include "tools/ToolItem.h"

class QToolButton;
class QVBoxLayout;

class AnnotationPropertyBar : public QFrame {
    Q_OBJECT

public:
    explicit AnnotationPropertyBar(QWidget *parent = nullptr);
public slots:
    void setTool(ToolType type);
private:
    QVBoxLayout    *m_layout;
};
