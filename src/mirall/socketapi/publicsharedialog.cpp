#include "publicsharedialog.h"
#include "ui_publicsharedialog.h"

PublicShareDialog::PublicShareDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PublicShareDialog)
{
    ui->setupUi(this);
}

PublicShareDialog::~PublicShareDialog()
{
    delete ui;
}

const QString PublicShareDialog::password() const
{
    return ui->passwordLineEdit->text();
}

const QString PublicShareDialog::validityLength() const
{
    return ui->lengthComboBox->currentText();
}

