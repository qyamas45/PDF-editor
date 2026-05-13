#pragma once
#include <QIcon>
#include <QString>

enum class ToolType { None, Select, Draw, Text };

struct ToolItem {
    ToolType type;
    QString  name;
    QIcon    icon;
};

Q_DECLARE_METATYPE(ToolType)
