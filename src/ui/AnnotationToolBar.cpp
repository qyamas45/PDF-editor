#include "AnnotationToolBar.h"

#include <QToolButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QPixmap>
#include <QCursor>
#include <QToolTip>
AnnotationToolBar::AnnotationToolBar(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(4, 8, 4, 8);
    m_layout->setSpacing(4);
    m_layout->setAlignment(Qt::AlignTop);

    setFixedWidth(52);
    setStyleSheet("background-color: #F5F5F5; border-right: 1px solid #CCCCCC;");
}
bool AnnotationToolBar::eventFilter(QObject *obj, QEvent *event)
{
 
    if (event->type() == QEvent::HoverEnter) {
        QToolButton *btn = qobject_cast<QToolButton*>(obj);
        if (btn) {
            ToolType type = btn->property("toolType").value<ToolType>();
            hoverToolTip(type);
        }
    }
    return QWidget::eventFilter(obj, event);
}

void AnnotationToolBar::setTools(const QVector<ToolItem> &tools)
{
    for (const ToolItem &item : tools) {
        QToolButton *btn = new QToolButton(this);
        btn->setIcon(item.icon);
        btn->setIconSize(QSize(28, 28));
        btn->setFixedSize(40, 40);
        btn->setToolTip(item.name);
        btn->setProperty("toolType", QVariant::fromValue(item.type));
        btn->setCheckable(false);

        connect(btn, &QToolButton::clicked, this, [this, type = item.type]() {
            onButtonClicked(type);
        });
        //if certain buttons, then an actual tooltip 
        //with instructions on how to use the tool 
        //(e.g. "Click and drag to draw annotation")
        if (item.type == ToolType::Draw)
        {   
          
            btn->setAttribute(Qt::WA_Hover);
            btn->installEventFilter(this);
        }
        m_layout->addWidget(btn);
        m_buttons.append(btn);
    }
    updateButtonStyles();
}

void AnnotationToolBar::setActiveTool(ToolType type)
{
    m_activeTool = type;
    updateButtonStyles();
}
void AnnotationToolBar::hoverToolTip(ToolType type)
{
    if (type == ToolType::Draw) {

        QToolTip::showText(QCursor::pos(), "Click and drag to draw annotation"); 
    }
    
}
void AnnotationToolBar::onButtonClicked(ToolType type)
{
    // Clicking the active tool deselects it (toggle off)
    m_activeTool = (m_activeTool == type) ? ToolType::None : type;
    updateButtonStyles();
    emit toolSelected(m_activeTool);
}
int AnnotationToolBar::buttonY(ToolType type) const
{
    for (QToolButton *btn : m_buttons)
        if (btn->property("toolType").value<ToolType>() == type)
            return btn->mapTo(parentWidget(), QPoint(0, 0)).y();
    return 0;
}
void AnnotationToolBar::updateButtonStyles()
{
    static const char *activeStyle =
        "QToolButton {"
        "  background: #D0E8FF;"
        "  border: 2px solid #4A90E2;"
        "  border-radius: 6px;"
        "  padding: 4px;"
        "}";
    static const char *inactiveStyle =
        "QToolButton {"
        "  background: transparent;"
        "  border: 2px solid transparent;"
        "  border-radius: 6px;"
        "  padding: 4px;"
        "}"
        "QToolButton:hover {"
        "  background: #E8E8E8;"
        "}";

    for (QToolButton *btn : m_buttons) {
        ToolType t = btn->property("toolType").value<ToolType>();
        btn->setStyleSheet(t == m_activeTool ? activeStyle : inactiveStyle);
    }
}

QIcon AnnotationToolBar::makeSelectIcon()
{
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    // Classic arrow cursor polygon (points toward top-left)
    // Body: tip → down left edge → inner notch → tail → back up hypotenuse
    QPolygonF cursor;
    cursor << QPointF(5, 4)    // 1: tip
           << QPointF(5, 23)   // 2: bottom-left (straight down)
           << QPointF(10, 18)  // 3: inner-left notch (cut in before tail)
           << QPointF(14, 28)  // 4: tail bottom
           << QPointF(17, 26)  // 5: tail right
           << QPointF(13, 16)  // 6: inner-right notch
           << QPointF(19, 14); // 7: outer-right → closes back to tip (hypotenuse)

    p.setPen(QPen(QColor("#3c5d7e"), 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(QColor(255, 255, 255, 220));
    p.drawPolygon(cursor);

    p.end();
    return QIcon(pm);
}

QIcon AnnotationToolBar::makeDrawIcon()
{
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    // Diagonal pen shaft
    p.setPen(QPen(QColor("#2C3E50"), 3.5, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(26, 4), QPointF(10, 20));

    // Nib triangle
    p.setPen(QPen(QColor("#2C3E50"), 1));
    p.setBrush(QColor("#2C3E50"));
    QPolygonF nib;
    nib << QPointF(8, 22) << QPointF(13, 19) << QPointF(10, 26);
    p.drawPolygon(nib);

    // Eraser cap (red dot at top)
    p.setPen(QPen(QColor("#E74C3C"), 3.5, Qt::SolidLine, Qt::RoundCap));
    p.drawPoint(QPointF(27, 3));

    p.end();
    return QIcon(pm);
}

QIcon AnnotationToolBar::makeTextIcon()
{
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    QFont font;
    font.setFamily("Arial");
    font.setPixelSize(22);
    font.setBold(true);
    p.setFont(font);
    p.setPen(QColor("#2C3E50"));
    p.drawText(QRect(0, 0, 32, 28), Qt::AlignCenter, "T");

    // Underline
    p.setPen(QPen(QColor("#2C3E50"), 2));
    p.drawLine(5, 30, 27, 30);

    p.end();
    return QIcon(pm);
}
