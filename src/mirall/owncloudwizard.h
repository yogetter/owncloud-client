/*
 * Copyright (C) by Duncan Mac-Vicar P. <duncan@kde.org>
 * Copyright (C) by Klaas Freitag <freitag@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#ifndef MIRALL_OWNCLOUDWIZARD_H
#define MIRALL_OWNCLOUDWIZARD_H

#include <QWizard>

#include "ui_owncloudsetuppage_ng.h"
#include "ui_owncloudwizardresultpage.h"

class QLabel;
class QVariant;

namespace Mirall {

class OwncloudSetupPage: public QWizardPage
{
    Q_OBJECT
public:
  OwncloudSetupPage();
  ~OwncloudSetupPage();

  virtual bool isComplete() const;
  virtual void initializePage();
  virtual int nextId() const;
  void setOCUrl( const QString& );
  void setOCUser( const QString& );
  void setAllowPasswordStorage( bool );
  bool validatePage();
  QString url() const;
  void setConnected(bool complete);

public slots:
  void setErrorString( const QString&  );

protected slots:
  void handleNewOcUrl(const QString& ocUrl);
  void setupCustomization();
  void slotToggleAdvanced(int state);

signals:
  void connectToOCUrl( const QString& );

private:
  Ui_OwncloudSetupPage _ui;
  QString _oCUrl;
  bool    _connected;
};

class OwncloudWizard: public QWizard
{
    Q_OBJECT
public:

    enum {
      Page_oCWelcome,
      Page_oCSetup,
      Page_Install
    };

    enum LogType {
      LogPlain,
      LogParagraph
    };

    OwncloudWizard(QWidget *parent = 0L);

    void setOCUrl( const QString& );
    void setOCUser( const QString& );

    void setupCustomMedia( QVariant, QLabel* );
    QString ocUrl() const;

    void enableFinishOnResultWidget(bool enable);

    void displayError( const QString& );

public slots:
    void appendToResultWidget( const QString& msg, LogType type = LogParagraph );
    void slotCurrentPageChanged( int );
    void showOCUrlLabel( bool );

    void successfullyConnected(bool);

signals:
    void clearPendingRequests();
    void connectToOCUrl( const QString& );

private:
    QString _configFile;
    QString _oCUrl;
    QString _oCUser;
};


/**
 * page for first launch only
 */
class OwncloudWelcomePage: public QWizardPage
{
    Q_OBJECT
public:
  OwncloudWelcomePage();

  virtual int nextId() const  { return OwncloudWizard::Page_oCSetup; }
};


/**
 * page to ask for the type of Owncloud to connect to
 */

/**
 * page to display the install result
 */
class OwncloudWizardResultPage : public QWizardPage
{
  Q_OBJECT
public:
  OwncloudWizardResultPage();
  ~OwncloudWizardResultPage();

  virtual bool isComplete() const;
  virtual void initializePage();

public slots:
  void appendResultText( const QString&, OwncloudWizard::LogType type = OwncloudWizard::LogParagraph );
  void showOCUrlLabel( const QString&, bool );

protected:
  void setupCustomization();

private:
  bool _complete;
  Ui_OwncloudWizardResultPage _ui;

};

} // ns Mirall

#endif
