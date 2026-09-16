#include "AnnotationToolBar.h"

#include <QToolButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QPixmap>
#include <QCursor>
#include <QToolTip>
/*
    File: AnnotationToolBar.cpp
    Purpose: This file implements the AnnotationToolBar class, which provides a 
             vertical toolbar with buttons for selecting annotation tools (Select, 
             Draw, Text) in the PDF editor application. It handles user interactions 
             with the toolbar buttons and emits signals when a tool is selected.
*/
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
/*
    Function name: eventFilter
    Purpose: To handle events for the tool buttons in the toolbar.
    Parameters: QObject *obj - the object that triggered the event.
                QEvent *event - the event to be processed.
    Returns: bool - true if the event was handled, false otherwise.
*/
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
/*
    Function name: setTools
    Purpose: To initialize the tool buttons in the toolbar with the provided tools.
    Parameters: const QVector<ToolItem> &tools - a list of tool items to 
                display in the toolbar.
    Returns: void - this function does not return a value.
*/
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
        //if (item.type == ToolType::Draw || item.type == ToolType::Erase)
        //{
        //    btn->setAttribute(Qt::WA_Hover);
        //    btn->installEventFilter(this);
        //}
        m_layout->addWidget(btn);
        m_buttons.append(btn);
    }
    updateButtonStyles();
}

/*
    Function name: setActiveTool
    Purpose: To set the currently active tool in the toolbar. This function is 
             called when a tool button is clicked. It updates the active tool state 
             and refreshes the button styles to reflect which tool is active.

    Parameters: ToolType type - the type of the tool that should be set as 
                active (e.g., Select, Draw, Text).

    Returns: void - this function does not return a value.

*/
void AnnotationToolBar::setActiveTool(ToolType type)
{
    m_activeTool = type;
    updateButtonStyles();
}
/*
    Function name: hoverToolTip
    Purpose: To show a tooltip with instructions when the user hovers over certain 
             tool buttons (e.g., the Draw tool). This function is called from the 
             event filter when a hover event is detected on a tool button.

    Parameters: ToolType type - the type of the tool for which to show the tooltip.

    Returns: void - this function does not return a value.
*/
void AnnotationToolBar::hoverToolTip(ToolType type)
{
    if (type == ToolType::Draw) {
        QToolTip::showText(QCursor::pos(), "Click and drag to draw annotation");
    } else if (type == ToolType::Erase) {
        QToolTip::showText(QCursor::pos(), "Click or drag to erase strokes and text");
    }

}
/*
    Function name: onButtonClicked
    Purpose: To handle the logic when a tool button is clicked. This function toggles 
             the active tool state (clicking the same tool again will deselect it) and 
             emits a signal to notify other components of the selected tool.

    Parameters: ToolType type - the type of the tool that was clicked.

    Returns: void - this function does not return a value.
*/
void AnnotationToolBar::onButtonClicked(ToolType type)
{
    // Clicking the active tool deselects it (toggle off)
    m_activeTool = (m_activeTool == type) ? ToolType::None : type;
    updateButtonStyles();
    emit toolSelected(m_activeTool);
}
/*
    Function name: buttonY and buttonX
    Purpose: To get the Y and X coordinates of a tool button in the toolbar, 
             relative to the parent widget (main window). This is used to position 
             the property bar next to the active tool button.

    Parameters: ToolType type - the type of the tool (Select, Draw, Text) for which 
                we want the button coordinates.

    Returns: int - the Y or X coordinate of the button in the parent widget's 
             coordinate system. If the button is not found, it returns
             0.

*/
int AnnotationToolBar::buttonY(ToolType type) const
{
    for (QToolButton *btn : m_buttons)
        if (btn->property("toolType").value<ToolType>() == type)
            return btn->mapTo(parentWidget(), QPoint(0, 0)).y();
    return 0;
}

int AnnotationToolBar::buttonX(ToolType type) const
{
    for (QToolButton *btn : m_buttons)
        if (btn->property("toolType").value<ToolType>() == type)
            return btn->mapTo(parentWidget(), QPoint(0, 0)).x()-5;
    return 0;
}

/*
    Function name: updateButtonStyles
    Purpose: To update the styles of all tool buttons in the toolbar based on 
             their active state.
    Parameters: None
    Returns: void
*/
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

/*
    Function name: makeSelectIcon
    Purpose: To create an icon for the Select tool button.
    Parameters: None
    Returns: QIcon - the icon for the Select tool button.
*/
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
/*
    Function name: makeDrawIcon
    Purpose: To create an icon for the Draw tool button.
    Parameters: None
    Returns: QIcon - the icon for the Draw tool button.
*/
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
/*
    Function name: makeTextIcon
    Purpose: To create an icon for the Text tool button.
    Parameters: None
    Returns: QIcon - the icon for the Text tool button.
*/
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
/*
    Function name: makeEraseIcon
    Purpose: To create an icon for the Erase tool button.
             The icon depicts a classic tilted eraser (pink body with a red band),
             recognizable at small sizes.
    Parameters: None
    Returns: QIcon - the icon for the Erase tool button.
*/
QIcon AnnotationToolBar::makeEraseIcon()
{
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.save();
    p.translate(16, 17);
    p.rotate(-25);

    // Main eraser body (light pink)
    p.setPen(QPen(QColor("#C0392B"), 1.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(QColor(255, 182, 193));
    p.drawRoundedRect(QRectF(-12, -6, 24, 12), 2, 2);

    // Dark red band on left third
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(231, 76, 60, 210));
    p.drawRoundedRect(QRectF(-12, -6, 8, 12), 2, 2);

    // Divider line between band and body
    p.setPen(QPen(QColor("#C0392B"), 1.0));
    p.drawLine(QPointF(-4, -6), QPointF(-4, 6));

    p.restore();

    p.end();
    return QIcon(pm);
}
QIcon AnnotationToolBar::makeShapeIcon()
{
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    // Draw a simple rectangle shape
    p.setPen(QPen(QColor("#2C3E50"), 2));
    p.setBrush(QColor("#3498DB"));
    p.drawRect(8, 8, 16, 16);

    p.end();
    return QIcon(pm);
}