#include "AnnotationPropertyBar.h"
#include <QToolButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QPixmap>
#include <QCursor>
#include <QToolTip>
AnnotationPropertyBar::AnnotationPropertyBar(QWidget *parent)
    : QFrame(parent)
    , m_layout(new QVBoxLayout(this))
{
 
    m_layout->setContentsMargins(8, 8, 8, 8);
    m_layout->setSpacing(6);
    m_layout->setAlignment(Qt::AlignTop);
    
    setFixedWidth(120);
    setFixedHeight(150); 
    setStyleSheet("background-color: #F5F5F5; border-left: 1px solid #CCCCCC;");
}

void AnnotationPropertyBar::setTool(ToolType type)
{   
    
}