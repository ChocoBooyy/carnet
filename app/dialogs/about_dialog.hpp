#pragma once

#include <QDialog>

namespace carnet {

class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);
};

}
