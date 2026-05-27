#pragma once
#include <iostream>
#include <QWidget>
#include <QVector>
#include "tools/ToolItem.h"
#include <QHoverEvent>
 
class QToolButton;
class QVBoxLayout;

class AnnotationToolBar : public QWidget {
    Q_OBJECT

public:
    explicit AnnotationToolBar(QWidget *parent = nullptr);

    void setTools(const QVector<ToolItem> &tools);
    void setActiveTool(ToolType type);
    void hoverToolTip(ToolType type);

    int buttonY(ToolType type) const;
    int buttonX(ToolType type) const;
    
    static QIcon makeSelectIcon();
    static QIcon makeDrawIcon();
    static QIcon makeTextIcon();
    static QIcon makeEraseIcon();

signals:
    void toolSelected(ToolType type);

private:
    QVBoxLayout          *m_layout;
    QVector<QToolButton*> m_buttons;
    ToolType              m_activeTool = ToolType::None;

    void onButtonClicked(ToolType type);
    void updateButtonStyles();
    bool eventFilter(QObject *obj, QEvent *event) override;
};
