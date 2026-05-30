#pragma once
#include <QFrame>
#include <QVector>

class QToolButton;
class QHBoxLayout;

class AnnotationRibbonBar : public QFrame {
    Q_OBJECT
public:
    explicit AnnotationRibbonBar(QWidget *parent = nullptr);

private:
    QHBoxLayout *m_layout;
    QWidget     *makeGroup(const QString &groupLabel, const QVector<QString> &labels);
    QWidget     *makeDivider();
    QToolButton *makeIconButton(const QString &label);
};
