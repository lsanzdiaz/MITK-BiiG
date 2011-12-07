/*=========================================================================
 
Program:   Medical Imaging & Interaction Toolkit
Module:    $RCSfile: mitkPropertyManager.cpp,v $
Language:  C++
Date:      $Date: 2005/06/28 12:37:25 $
Version:   $Revision: 1.12 $
 
Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.
 
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.
 
=========================================================================*/

#ifndef QmitkMouseModeSwitcher_h
#define QmitkMouseModeSwitcher_h

#include "QmitkExports.h"

#include <QtGui>

namespace mitk
{
  class MouseModeSwitcher;
}

/**
  \brief Qt toolbar representing mitk::MouseModeSwitcher.

  Provides buttons for the interaction modes defined in mitk::MouseModeSwitcher
  and communicates with this non-graphical class.

  Can be used in a GUI to provide a mouse mode selector to the user.
*/
class QMITK_EXPORT QmitkMouseModeSwitcher : public QToolBar
{
  Q_OBJECT

  public:

    QmitkMouseModeSwitcher( QWidget* parent = 0 );
    virtual ~QmitkMouseModeSwitcher();

  signals:

    /**
      \brief Mode activated.

      This signal is needed for other GUI element to react appropriately.
      Sadly this is needed to provide "normal" functionality of QmitkStdMultiWidget,
      because this must enable/disable automatic reaction of SliceNavigationControllers
      to mouse clicks - depending on which mode is active.
    */
    void modeActivated(int id); // TODO change int to enum of MouseModeSwitcher

  protected slots:

    void modeSelectedByUser();
    void addButton( int id, const QString& toolName, const QIcon& icon, bool on = false ); // TODO change int to enum of MouseModeSwitcher
    void setMouseModeSwitcher( mitk::MouseModeSwitcher* );

  protected:

    QActionGroup* m_ActionGroup;
    mitk::MouseModeSwitcher* m_MouseModeSwitcher;
};

#endif

