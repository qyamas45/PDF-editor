#pragma once
#include <QIcon>
#include <QString>

enum class ToolType { None, Select, Draw, Text, Erase, Shape };

struct ToolItem {
    ToolType type;
    QString  name;
    QIcon    icon;
};

Q_DECLARE_METATYPE(ToolType)
