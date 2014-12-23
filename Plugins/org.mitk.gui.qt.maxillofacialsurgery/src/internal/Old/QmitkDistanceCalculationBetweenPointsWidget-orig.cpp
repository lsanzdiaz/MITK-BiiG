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

#include "QmitkDistanceCalculationBetweenPointsWidget.h"


QmitkDistanceCalculationBetweenPointsWidget::QmitkDistanceCalculationBetweenPointsWidget(QWidget* parent)
: QWidget(parent), m_Controls(NULL),m_MultiWidget(NULL), m_ReferencePositionNode(NULL), m_MeasurementPositionNode(NULL)
{
  CreateQtPartControl(this);
}


QmitkDistanceCalculationBetweenPointsWidget::~QmitkDistanceCalculationBetweenPointsWidget()
{
  m_Controls = NULL;
}


void QmitkDistanceCalculationBetweenPointsWidget::CreateQtPartControl(QWidget *parent)
{
  if (!m_Controls)
  {
    // create GUI widgets
    m_Controls = new Ui::QmitkDistanceCalculationBetweenPointsWidget;
    m_Controls->setupUi(parent);

    this->CreateConnections();
  }
}


void QmitkDistanceCalculationBetweenPointsWidget::CreateConnections()
{
	connect((QObject*)(m_Controls->m_AddReferencePositionBtn), SIGNAL(clicked()), this, SIGNAL(AddedReferencePosition()));
	connect((QObject*)(m_Controls->m_AddMeasurementPositionBtn), SIGNAL(clicked()), this, SIGNAL(AddedMeasurementPosition()));
	connect((QObject*)(m_Controls->m_PerformDistanceCalculationBtn), SIGNAL(clicked()), this, SIGNAL(CalculateDistance()));
}

void QmitkDistanceCalculationBetweenPointsWidget::SetReferencePositionNode(mitk::DataNode::Pointer ReferencePositionNode)
{
   m_Controls->m_ReferencePosition->SetMultiWidget(m_MultiWidget); // pass multiWidget to pointListWidget
}

void QmitkDistanceCalculationBetweenPointsWidget::SetMeasurementPositionNode(mitk::DataNode::Pointer MeasurementPositionNode)
{
	m_Controls->m_MeasurementPosition->SetMultiWidget(m_MultiWidget); // pass multiWidget to pointListWidget
}

void QmitkDistanceCalculationBetweenPointsWidget::SetMultiWidget( QmitkStdMultiWidget* multiWidget )
{
  m_MultiWidget=multiWidget;
}

void QmitkDistanceCalculationBetweenPointsWidget::AddSliceNavigationController(mitk::SliceNavigationController* snc)
{
  m_Controls->m_ReferencePosition->AddSliceNavigationController(snc);
  m_Controls->m_MeasurementPosition->AddSliceNavigationController(snc);
}

mitk::DataNode::Pointer QmitkDistanceCalculationBetweenPointsWidget::GetMeasurementPositionNode()
{
  return m_MeasurementPositionNode;
}

mitk::DataNode::Pointer QmitkDistanceCalculationBetweenPointsWidget::GetReferencePositionNode()
{
	return m_ReferencePositionNode;
}
