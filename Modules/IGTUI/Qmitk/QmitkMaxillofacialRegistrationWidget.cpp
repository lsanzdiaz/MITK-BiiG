/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include "QmitkMaxillofacialRegistrationWidget.h"


#define FRW_LOG MITK_INFO("Fiducial Registration Widget")
#define FRW_WARN MITK_WARN("Fiducial Registration Widget")
#define FRW_DEBUG MITK_DEBUG("Fiducial Registration Widget")

/* VIEW MANAGEMENT */
QmitkMaxillofacialRegistrationWidget::QmitkMaxillofacialRegistrationWidget(QWidget* parent)
: QWidget(parent), m_Controls(NULL),m_MultiWidget(NULL), m_ImageFiducialsNode(NULL), m_TrackerFiducialsNode(NULL)
{
  CreateQtPartControl(this);
}


QmitkMaxillofacialRegistrationWidget::~QmitkMaxillofacialRegistrationWidget()
{
  m_Controls = NULL;
}


void QmitkMaxillofacialRegistrationWidget::CreateQtPartControl(QWidget *parent)
{
  if (!m_Controls)
  {
    // create GUI widgets
    m_Controls = new Ui::QmitkMaxillofacialRegistrationWidget;
    m_Controls->setupUi(parent);

    // hide additional image fiducial button
    m_Controls->m_AddImageFiducialBtn->setHidden(true);

    m_Controls->m_spaceHolderGroupBox->setStyleSheet("QGroupBox {border: 0px transparent;}");
    m_Controls->m_spaceHolderGroupBox2->setStyleSheet("QGroupBox {border: 0px transparent;}");

    this->CreateConnections();
  }
}


void QmitkMaxillofacialRegistrationWidget::CreateConnections()
{
  connect( (QObject*)(m_Controls->m_AddTrackingFiducialBtn), SIGNAL(clicked()), this, SIGNAL(AddedTrackingFiducial()) );
  connect( (QObject*)(m_Controls->m_AddImageFiducialBtn), SIGNAL(clicked()), this, SIGNAL(AddedImageFiducial()) );
  connect( (QObject*)(m_Controls->m_RegisterFiducialsBtn), SIGNAL(clicked()), this, SIGNAL(PerformFiducialRegistration()) );
  connect( (QObject*)(m_Controls->m_UseICPRegistration), SIGNAL(toggled(bool)), this, SIGNAL(FindFiducialCorrespondences(bool)) );

  //unselects the edit button of the other widget if one is selected
  connect( (QObject*)(m_Controls->m_RegistrationImagePoints), SIGNAL(EditPointSets(bool)), this, SLOT(DisableEditButtonRegistrationTrackingPoints(bool)));
  connect( (QObject*)(m_Controls->m_RegistrationTrackingPoints), SIGNAL(EditPointSets(bool)), this, SLOT(DisableEditButtonRegistrationImagePoints(bool)));
}

void QmitkMaxillofacialRegistrationWidget::DisableEditButtonRegistrationImagePoints(bool activated)
{
if (activated) m_Controls->m_RegistrationImagePoints->UnselectEditButton();
}
void QmitkMaxillofacialRegistrationWidget::DisableEditButtonRegistrationTrackingPoints(bool activated)
{
if (activated) m_Controls->m_RegistrationTrackingPoints->UnselectEditButton();
}

void QmitkMaxillofacialRegistrationWidget::SetWidgetAppearanceMode(WidgetAppearanceMode widgetMode)
{
  if (widgetMode==LANDMARKMODE)
  {
    this->HideContinousRegistrationRadioButton(true);
    this->HideStaticRegistrationRadioButton(true);
    this->HideFiducialRegistrationGroupBox();
    this->HideUseICPRegistrationCheckbox(true);
    this->HideImageFiducialButton(false);
    this->m_Controls->sourceLandmarksGroupBox->setTitle("Target/Reference landmarks");
    this->m_Controls->targetLandmarksGroupBox->setTitle("Source Landmarks");
    this->m_Controls->m_AddImageFiducialBtn->setText("Add target landmark");
    this->m_Controls->m_AddTrackingFiducialBtn->setText("Add source landmark");
  }
  else if (widgetMode==FIDUCIALMODE)
  {
    this->HideContinousRegistrationRadioButton(false);
    this->HideStaticRegistrationRadioButton(false);
    this->HideFiducialRegistrationGroupBox();
    this->HideUseICPRegistrationCheckbox(false);
    this->HideImageFiducialButton(true);
    this->m_Controls->sourceLandmarksGroupBox->setTitle("Image fiducials");
    this->m_Controls->targetLandmarksGroupBox->setTitle("OR fiducials");
    this->m_Controls->m_AddImageFiducialBtn->setText("Add image fiducial");
    this->m_Controls->m_AddTrackingFiducialBtn->setText("Add current instrument position");
  }
}

void QmitkMaxillofacialRegistrationWidget::SetQualityDisplayText( QString text )
{
  if (text == NULL)
    return;
  m_Controls->m_RegistrationQualityDisplay->setText(text); // set text on the QLabel
}

bool QmitkMaxillofacialRegistrationWidget::UseICPIsChecked()
{
  if(m_Controls->m_UseICPRegistration->isChecked())
    return true;
  else
    return false;
}

void QmitkMaxillofacialRegistrationWidget::SetImageFiducialsNode( mitk::DataNode::Pointer imageFiducialsNode )
{
  if(imageFiducialsNode.IsNull())
  {
    FRW_WARN<< "tracker fiducial node is NULL";
    return;
  }
  m_Controls->m_RegistrationImagePoints->SetPointSetNode(imageFiducialsNode); // pass node to pointListWidget
  if(m_MultiWidget == NULL)
  {
    FRW_WARN<< "stdMultiWidget is NULL";
    return;
  }
  m_Controls->m_RegistrationImagePoints->SetMultiWidget(m_MultiWidget); // pass multiWidget to pointListWidget
}

void QmitkMaxillofacialRegistrationWidget::SetTrackerFiducialsNode( mitk::DataNode::Pointer trackerFiducialsNode )
{
  if(trackerFiducialsNode.IsNull())
  {
    FRW_WARN<< "tracker fiducial node is NULL";
    return;
  }
  m_Controls->m_RegistrationTrackingPoints->SetPointSetNode(trackerFiducialsNode); // pass node to pointListWidget
  if(m_MultiWidget == NULL)
  {
    FRW_WARN<< "stdMultiWidget is NULL";
    return;
  }
  m_Controls->m_RegistrationTrackingPoints->SetMultiWidget(m_MultiWidget); // pass multiWidget to pointListWidget
}

void QmitkMaxillofacialRegistrationWidget::SetMultiWidget( QmitkStdMultiWidget* multiWidget )
{
  m_MultiWidget=multiWidget;
}

void QmitkMaxillofacialRegistrationWidget::AddSliceNavigationController(mitk::SliceNavigationController* snc)
{
  m_Controls->m_RegistrationTrackingPoints->AddSliceNavigationController(snc);
  m_Controls->m_RegistrationImagePoints->AddSliceNavigationController(snc);
}

mitk::DataNode::Pointer QmitkMaxillofacialRegistrationWidget::GetImageFiducialsNode()
{
  return m_ImageFiducialsNode;
}

mitk::DataNode::Pointer QmitkMaxillofacialRegistrationWidget::GetTrackerFiducialsNode()
{
  return m_TrackerFiducialsNode;
}

void QmitkMaxillofacialRegistrationWidget::HideStaticRegistrationRadioButton( bool on )
{
  m_Controls->m_rbStaticRegistration->setHidden(on);
  HideFiducialRegistrationGroupBox();
}

void QmitkMaxillofacialRegistrationWidget::HideContinousRegistrationRadioButton( bool on )
{
  m_Controls->m_rbContinousRegistration->setHidden(on);
  HideFiducialRegistrationGroupBox();
}

void QmitkMaxillofacialRegistrationWidget::HideFiducialRegistrationGroupBox()
{
  if (m_Controls->m_rbStaticRegistration->isHidden() && m_Controls->m_rbContinousRegistration->isHidden())
  {
    m_Controls->m_gbFiducialRegistration->setHidden(true);
  }
  else
  {
    m_Controls->m_gbFiducialRegistration->setHidden(false);
  }
}

void QmitkMaxillofacialRegistrationWidget::HideUseICPRegistrationCheckbox( bool on )
{
  m_Controls->m_UseICPRegistration->setHidden(on);
}

void QmitkMaxillofacialRegistrationWidget::HideImageFiducialButton( bool on )
{
  m_Controls->m_AddImageFiducialBtn->setHidden(on);
  AdjustButtonSpacing();

}

void QmitkMaxillofacialRegistrationWidget::HideTrackingFiducialButton( bool on )
{
  m_Controls->m_AddTrackingFiducialBtn->setHidden(on);
  AdjustButtonSpacing();
}

void QmitkMaxillofacialRegistrationWidget::AdjustButtonSpacing()
{
  if (m_Controls->m_AddImageFiducialBtn->isHidden() && m_Controls->m_AddTrackingFiducialBtn->isHidden())
  {
    m_Controls->m_spaceHolderGroupBox->setHidden(true);
    m_Controls->m_spaceHolderGroupBox2->setHidden(true);
  }
  else
  {
    m_Controls->m_spaceHolderGroupBox->setHidden(false);
    m_Controls->m_spaceHolderGroupBox2->setHidden(false);
  }
}

void QmitkMaxillofacialRegistrationWidget::SetSourceLandmarkName(QString sourceLandmarkName)
{
  QString groupBoxTitle = sourceLandmarkName;
  groupBoxTitle.append(" Landmarks");
  m_Controls->sourceLandmarksGroupBox->setTitle(groupBoxTitle);
  QString buttonText = "Add ";
  buttonText.append(sourceLandmarkName);
  buttonText.append(" Landmark");
  m_Controls->m_AddImageFiducialBtn->setText(buttonText);
}

void QmitkMaxillofacialRegistrationWidget::SetTargetLandmarkName(QString targetLandmarkName)
{
  QString groupBoxTitle = targetLandmarkName;
  groupBoxTitle.append(" Landmarks");
  m_Controls->targetLandmarksGroupBox->setTitle(groupBoxTitle);
  QString buttonText = "Add ";
  buttonText.append(targetLandmarkName);
  buttonText.append(" Landmark");
  m_Controls->m_AddTrackingFiducialBtn->setText(buttonText);
}
