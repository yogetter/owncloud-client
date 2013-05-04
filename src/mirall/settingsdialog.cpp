/*
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "mirall/theme.h"
#include "mirall/generalsettings.h"
#include "mirall/accountsettings.h"
#include "mirall/progressdispatcher.h"

#include <QLabel>
#include <QStandardItemModel>
#include <QPushButton>
#include <QDebug>

namespace Mirall {

QIcon createDummy() {
    QIcon icon;
    QPixmap p(32,32);
    p.fill(Qt::transparent);
    icon.addPixmap(p);
    return icon;
}

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::SettingsDialog)
{
    _ui->setupUi(this);
    connect(_ui->labelWidget, SIGNAL(currentRowChanged(int)),
            SLOT(handleItemClick(int)));
    connect(_ui->labelWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            SLOT(checkResetToOldItem(QListWidgetItem*,QListWidgetItem*)));

    connect( ProgressDispatcher::instance(), SIGNAL(folderUploadProgress(QString,QString,long,long)),
             SLOT(slotFolderUploadProgress(QString,QString,long,long)));

    QListWidgetItem *general = new QListWidgetItem(tr("General"), _ui->labelWidget);
    general->setSizeHint(QSize(0, 32));
    _ui->labelWidget->addItem(general);
    GeneralSettings *generalSettings = new GeneralSettings;
    connect(generalSettings, SIGNAL(resizeToSizeHint()), SLOT(resizeToSizeHint()));
    _ui->stack->addWidget(generalSettings);

    _addItem = new QListWidgetItem(tr("Add Account"), _ui->labelWidget);
    _addItem->setSizeHint(QSize(0, 32));
    _ui->labelWidget->addItem(_addItem);

    _accountSettings = new AccountSettings;
    addAccount(tr("Account"), _accountSettings);

    QPushButton *closeButton = _ui->buttonBox->button(QDialogButtonBox::Close);
    connect(closeButton, SIGNAL(pressed()), SLOT(accept()));

    _ui->labelWidget->setCurrentRow(_ui->labelWidget->row(general));
}

SettingsDialog::~SettingsDialog()
{
    delete _ui;
}

void SettingsDialog::addAccount(const QString &title, QWidget *widget)
{
    QListWidgetItem *item = new QListWidgetItem(title);
    item->setSizeHint(QSize(0, 32));
    qDebug() << Q_FUNC_INFO << _ui->stack->count();
    _ui->labelWidget->insertItem(_ui->labelWidget->count()-1, item);
    _ui->stack->addWidget(widget);
    qDebug() << Q_FUNC_INFO << _ui->stack->count();

}

void SettingsDialog::handleItemClick(int row)
{
    if (row != _ui->labelWidget->count()-1)
        _ui->stack->setCurrentIndex(row);
}

void SettingsDialog::checkResetToOldItem(QListWidgetItem *current, QListWidgetItem *previous)
{
    // bit of a hack...
    if (current == previous)
        qDebug () << "current == previous";
    if (current == _addItem) {
        int row = _ui->labelWidget->row(previous);
        QMetaObject::invokeMethod(this, "asyncSwitch", Qt::QueuedConnection, Q_ARG(int, row));
        qDebug() << "TODO: Show Account Wizard";
    }

}

void SettingsDialog::asyncSwitch(int row)
{
    _ui->labelWidget->setCurrentRow(row);
}

void SettingsDialog::slotFolderUploadProgress( const QString& folderAlias, const QString& file, long p1, long p2)
{
    qDebug() << " SettingsDialog: XX - File " << file << p1 << p2;
    _accountSettings->slotSetFolderProgress(folderAlias, file, p1, p2);
}


} // namespace Mirall
