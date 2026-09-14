#include "dialogs/about_dialog.hpp"

#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

#include "carnet/database.hpp"

namespace carnet {

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("About Carnet"));

    auto* title = new QLabel(QString("<b>Carnet</b> %1").arg(CARNET_VERSION), this);
    QFont font = title->font();
    font.setPointSizeF(font.pointSizeF() * 1.4);
    title->setFont(font);
    auto* description = new QLabel(
        tr("A personal budget app: import your bank's CSV export, sort it with your own rules, "
           "and see where the money went each month. Everything stays in one file on this computer."),
        this);
    description->setWordWrap(true);
    auto* credits = new QLabel(
        tr("Open source under the MIT license. Built with Qt %1 (LGPL v3) and SQLite %2 (public domain).")
            .arg(qVersion(), QString::fromStdString(sqliteVersion())),
        this);
    credits->setWordWrap(true);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(credits);
    layout->addWidget(buttons);
    setFixedWidth(420);

    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
}

}
