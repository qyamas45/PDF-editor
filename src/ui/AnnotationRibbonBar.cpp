#include "AnnotationRibbonBar.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QFrame>

AnnotationRibbonBar::AnnotationRibbonBar(QWidget *parent)
    : QFrame(parent)
    , m_layout(new QHBoxLayout(this))
{
    setFixedHeight(68);
    setStyleSheet(
        "AnnotationRibbonBar {"
        "  background-color: #F0F0F0;"
        "  border-bottom: 1px solid #CCCCCC;"
        "}"
    );
    m_layout->setContentsMargins(6, 4, 6, 4);
    m_layout->setSpacing(0);

    m_layout->addWidget(makeGroup("File",   {"Open",  "Save"}));
    m_layout->addWidget(makeDivider());
    m_layout->addWidget(makeGroup("Tools",  {"Sel",   "Draw",  "Text",  "Erase"}));
    m_layout->addWidget(makeDivider());
    m_layout->addWidget(makeGroup("View",   {"Z+",    "Z-",    "Fit"}));
    m_layout->addWidget(makeDivider());
    m_layout->addWidget(makeGroup("Format", {"Color", "Width", "Font"}));
    m_layout->addStretch(1);
}

QToolButton *AnnotationRibbonBar::makeIconButton(const QString &label)
{
    QToolButton *btn = new QToolButton(this);
    btn->setText(label);
    btn->setFixedSize(36, 36);
    btn->setToolTip(label);
    btn->setStyleSheet(
        "QToolButton {"
        "  background: transparent;"
        "  border: 1px solid transparent;"
        "  border-radius: 4px;"
        "  font-size: 9px;"
        "  color: #333333;"
        "}"
        "QToolButton:hover {"
        "  background: #E0E0E0;"
        "  border-color: #BBBBBB;"
        "}"
        "QToolButton:pressed { background: #D0D0D0; }"
    );
    return btn;
}

QWidget *AnnotationRibbonBar::makeGroup(const QString & /*groupLabel*/,
                                         const QVector<QString> &labels)
{
    QWidget     *container = new QWidget(this);
    QVBoxLayout *vbox      = new QVBoxLayout(container);
    vbox->setContentsMargins(4, 2, 4, 2);
    vbox->setSpacing(2);

    QWidget     *btnRow  = new QWidget(container);
    QHBoxLayout *btnHBox = new QHBoxLayout(btnRow);
    btnHBox->setContentsMargins(0, 0, 0, 0);
    btnHBox->setSpacing(2);
    for (const QString &lbl : labels)
        btnHBox->addWidget(makeIconButton(lbl));
    vbox->addWidget(btnRow);


    return container;
}

QWidget *AnnotationRibbonBar::makeDivider()
{
    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setFixedWidth(1);
    line->setStyleSheet("color: #CCCCCC;");
    return line;
}
