#include "AnnotationPropertyBar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
/*
    Function name: AnnotationPropertyBar
    Purpose: Constructor for the AnnotationPropertyBar class.
*/
AnnotationPropertyBar::AnnotationPropertyBar(QWidget *parent)
    : QFrame(parent)
    , m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(8, 8, 8, 8);
    m_layout->setSpacing(6);
    m_layout->setAlignment(Qt::AlignTop);

    setFixedWidth(180);
    setStyleSheet("background-color: #F5F5F5; border-left: 1px solid #CCCCCC;");
}

void AnnotationPropertyBar::clearPanel()
{
    while (QLayoutItem *item = m_layout->takeAt(0)) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }
}
/*
    Function name: makeColorSwatch
    Purpose: Creates a color swatch button for the property bar.
    Details: This function creates a QPushButton with a 
             fixed size and background color based on the provided QColor.
*/
static QPushButton *makeColorSwatch(const QColor &color)
{
    QPushButton *btn = new QPushButton;
    btn->setFixedSize(18, 18);
    const QString hex = color.name();
    btn->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  border: 1px solid #888;"
        "  border-radius: 9px;"
        "}"
        "QPushButton:hover { border: 2px solid #333; }"
    ).arg(hex));
    return btn;
}

static QLabel *makeLabel(const QString &text, bool bold = false)
{
    QLabel *lbl = new QLabel(text);
    lbl->setStyleSheet(bold
        ? "font-size: 11px; font-weight: bold; color: #333;"
        : "font-size: 10px; color: #666;");
    return lbl;
}
/*
    Function name: buildDrawPanel
    Purpose: Configures the property bar with controls for drawing annotations.
    Details: This function adds a label for the "Draw" tool, followed by 
             controls for selecting stroke color and width. It creates 
             color swatches for predefined colors and buttons for common 
             stroke widths. Each control is connected to the appropriate 
             signal to notify changes in stroke properties. Finally, it 
             sets a fixed height for the property bar to accommodate the 
             added controls.
*/
void AnnotationPropertyBar::buildDrawPanel()
{
    m_layout->addWidget(makeLabel("Draw", true));

    // color row
    m_layout->addWidget(makeLabel("Color"));

    const QList<QColor> colors = {
        QColor("#DC3232"),
        QColor("#1E1EB4"),
        QColor("#222222"),
        QColor("#2E7D32"),
    };

    QWidget *colorRow = new QWidget;
    QHBoxLayout *colorHBox = new QHBoxLayout(colorRow);
    colorHBox->setContentsMargins(0, 0, 0, 0);
    colorHBox->setSpacing(4);

    for (const QColor &c : colors) {
        QPushButton *btn = makeColorSwatch(c);
        connect(btn, &QPushButton::clicked, this, [this, c]() {
            emit strokeColorChanged(c);
        });
        colorHBox->addWidget(btn);
    }
    colorHBox->addStretch();
    m_layout->addWidget(colorRow);

    // Width row
    m_layout->addWidget(makeLabel("Width"));

    QSpinBox *widthSpin = new QSpinBox;
    widthSpin->setRange(1, 20);
    widthSpin->setValue(1);
    widthSpin->setSuffix(" px");
    connect(widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
        emit strokeWidthChanged(static_cast<qreal>(v));
    });

    struct WidthOpt { QString label; int value; };
    const QList<WidthOpt> widths = { {"—", 1}, {"━", 3}, {"█", 6} };

    QWidget *widthRow = new QWidget;
    QHBoxLayout *widthHBox = new QHBoxLayout(widthRow);
    widthHBox->setContentsMargins(0, 0, 0, 0);
    widthHBox->setSpacing(4);

    for (const WidthOpt &opt : widths) {
        auto *btn = new QPushButton(opt.label);
        btn->setFixedSize(28, 22);
        btn->setStyleSheet(
            "QPushButton {"
            "  background: #E0E0E0; border: 1px solid #AAA;"
            "  border-radius: 3px; font-size: 11px;"
            "}"
            "QPushButton:hover { background: #C8D8F0; border-color: #4A90E2; }"
        );
        connect(btn, &QPushButton::clicked, this, [widthSpin, v = opt.value]() {
            widthSpin->setValue(v);
        });
        widthHBox->addWidget(btn);
    }
    widthHBox->addWidget(widthSpin);
    widthHBox->addStretch();
    m_layout->addWidget(widthRow);
    setFixedHeight(170);
}
/*
    Function name: buildTextPanel
    Purpose: Configures the property bar with controls for text annotations.
    Details: This function adds a label for the "Text" tool, followed by 
             controls for selecting text color and font size. It creates 
             color swatches for predefined colors and buttons for common 
             font sizes. Each control is connected to the appropriate signal 
             to notify changes in text properties. Finally, it sets a fixed 
             height for the property bar to accommodate the added controls.
*/
void AnnotationPropertyBar::buildTextPanel()
{
    m_layout->addWidget(makeLabel("Text", true));

    // Color row
    m_layout->addWidget(makeLabel("Color"));

    const QList<QColor> colors = {
        QColor("#1E1EB4"),
        QColor("#222222"),
        QColor("#DC3232"),
        QColor("#2E7D32"),
    };

    QWidget *colorRow = new QWidget;
    QHBoxLayout *colorHBox = new QHBoxLayout(colorRow);
    colorHBox->setContentsMargins(0, 0, 0, 0);
    colorHBox->setSpacing(4);

    for (const QColor &c : colors) {
        QPushButton *btn = makeColorSwatch(c);
        connect(btn, &QPushButton::clicked, this, [this, c]() {
            emit textColorChanged(c);
        });
        colorHBox->addWidget(btn);
    }
    colorHBox->addStretch();
    m_layout->addWidget(colorRow);

    // Font size row    
    m_layout->addWidget(makeLabel("Size"));

    struct SizeOpt { QString label; int value; };
    const QList<SizeOpt> sizes = { {"S", 12}, {"M", 16}, {"L", 24} };

    QWidget *sizeRow = new QWidget;
    QHBoxLayout *sizeHBox = new QHBoxLayout(sizeRow);
    sizeHBox->setContentsMargins(0, 0, 0, 0);
    sizeHBox->setSpacing(4);

    for (const SizeOpt &opt : sizes) {
        QPushButton *btn = new QPushButton(opt.label);
        btn->setFixedSize(28, 22);
        btn->setStyleSheet(
            "QPushButton {"
            "  background: #E0E0E0; border: 1px solid #AAA;"
            "  border-radius: 3px; font-size: 11px;"
            "}"
            "QPushButton:hover { background: #C8D8F0; border-color: #4A90E2; }"
        );
        connect(btn, &QPushButton::clicked, this, [this, sz = opt.value]() {
            emit fontSizeChanged(sz);
        });
        sizeHBox->addWidget(btn);
    }

    // Add a spin box for custom font size
    QSpinBox *sizeSpin = new QSpinBox;
    sizeSpin->setRange(8, 72);
    sizeSpin->setValue(8);
    sizeSpin->setSuffix(" pt");
    connect(sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
        emit fontSizeChanged(v);
    });
    sizeHBox->addWidget(sizeSpin);

    sizeHBox->addStretch();
    m_layout->addWidget(sizeRow);

    setFixedHeight(170);
}
/*
    Function name: buildErasePanel
    Purpose: Configures the property bar with instructions for using the eraser tool.
    Details: This function adds a label indicating the "Erase" tool 
             and a descriptive text to guide users on how to use 
             the eraser functionality. It sets a fixed height 
             for the property bar to accommodate the content.
*/
void AnnotationPropertyBar::buildErasePanel()
{
    m_layout->addWidget(makeLabel("Erase", true));

    QLabel *info = new QLabel("Click and drag to erase parts of strokes.");
    info->setWordWrap(true);
    info->setStyleSheet("font-size: 10px; color: #666;");
    m_layout->addWidget(info);

    m_layout->addWidget(makeLabel("Size"));

    QSpinBox *sizeSpin = new QSpinBox;
    sizeSpin->setRange(1, 50);
    sizeSpin->setValue(8);          // matches the overlay's default radius
    sizeSpin->setSuffix(" px");
    connect(sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
        emit eraserRadiusChanged(static_cast<qreal>(v));
    });
    m_layout->addWidget(sizeSpin);

    setFixedHeight(120);
}
/*
    Function name: setTool
    Purpose: Updates the property bar based on the selected annotation tool.
    Details: This function first clears any existing controls from the property bar. 
             It then checks the type of tool selected (Draw, Text, Erase) and 
             calls the corresponding function to build the appropriate control panel. 
             If an unrecognized tool type is provided, it simply sets a default height 
             for the property bar without adding any controls.
*/
void AnnotationPropertyBar::setTool(ToolType type)
{
    clearPanel();

    if (type == ToolType::Draw)
        buildDrawPanel();
    else if (type == ToolType::Text)
        buildTextPanel();
    else if (type == ToolType::Erase)
        buildErasePanel();
    else
        setFixedHeight(40);
}
