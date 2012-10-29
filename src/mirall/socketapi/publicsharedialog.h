#ifndef PUBLICSHAREDIALOG_H
#define PUBLICSHAREDIALOG_H

#include <QDialog>

namespace Ui {
class PublicShareDialog;
}

class PublicShareDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PublicShareDialog(QWidget *parent = 0);
    ~PublicShareDialog();

    const QString password() const;
    const QString validityLength() const;

private:
    Ui::PublicShareDialog *ui;
};

#endif // PUBLICSHAREDIALOG_H
