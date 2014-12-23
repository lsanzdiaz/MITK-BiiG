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
#include <fstream>
#include <tinyxml.h>
#include <itksys/SystemTools.hxx>

#define FRW_LOG MITK_INFO("DistanceCalculationBetweenPointsWidget")
#define FRW_WARN MITK_WARN("DistanceCalculationBetweenPointsWidget")
#define FRW_DEBUG MITK_DEBUG("DistanceCalculationBetweenPointsWidget")

/* VIEW MANAGEMENT */
QmitkDistanceCalculationBetweenPointsWidget::QmitkDistanceCalculationBetweenPointsWidget(QWidget* parent)
: QWidget(parent), m_Controls(NULL),m_MultiWidget(NULL), m_ReferenceNode(NULL), m_MeasurementNode(NULL)
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

    m_Controls->m_spaceHolderGroupBox->setStyleSheet("QGroupBox {border: 0px transparent;}");
    m_Controls->m_spaceHolderGroupBox2->setStyleSheet("QGroupBox {border: 0px transparent;}");

    this->CreateConnections();
  }
}


void QmitkDistanceCalculationBetweenPointsWidget::CreateConnections()
{
  connect((QObject*)(m_Controls->m_AddReferencePositionBtn), SIGNAL(clicked()), this, SIGNAL(AddedReferencePosition()));
  connect((QObject*)(m_Controls->m_AddMeasurementPositionBtn), SIGNAL(clicked()), this, SIGNAL(AddedMeasurementPosition()));
  connect((QObject*)(m_Controls->m_CalculateDistanceBtn), SIGNAL(clicked()), this, SIGNAL(DemandDistanceCalculation()));
  connect((QObject*)(m_Controls->m_SaveDataBtn), SIGNAL(clicked()), this, SLOT(OnSaveData()));
}

void QmitkDistanceCalculationBetweenPointsWidget::SetQualityDisplayText( QString text )
{
  if (text == NULL)
    return;
}

void QmitkDistanceCalculationBetweenPointsWidget::SetReferencePositionNode(mitk::DataNode::Pointer ReferenceNode)
{
if (ReferenceNode.IsNull())
  {
	  FRW_WARN << "tracker fiducial node is NULL";
	  return;
  }
    m_ReferenceNode = ReferenceNode;
	m_Controls->m_ReferencePoints->SetPointSetNode(m_ReferenceNode); // pass node to pointListWidget
  if (m_MultiWidget == NULL)
  {
	  FRW_WARN << "stdMultiWidget is NULL";
	  return;
  }
  m_Controls->m_ReferencePoints->SetMultiWidget(m_MultiWidget); // pass multiWidget to pointListWidget
}

void QmitkDistanceCalculationBetweenPointsWidget::SetMeasurementPositionNode(mitk::DataNode::Pointer MeasurementNode)
{
	if (MeasurementNode.IsNull())
	{
		FRW_WARN << "tracker fiducial node is NULL";
		return;
	}
	m_MeasurementNode = MeasurementNode;
	m_Controls->m_MeasurementPoints->SetPointSetNode(m_MeasurementNode); // pass node to pointListWidget
	if (m_MultiWidget == NULL)
	{
		FRW_WARN << "stdMultiWidget is NULL";
		return;
	}
	m_Controls->m_MeasurementPoints->SetMultiWidget(m_MultiWidget); // pass multiWidget to pointListWidget
}


void QmitkDistanceCalculationBetweenPointsWidget::AddDistanceToList(double distance)
{
	QListWidgetItem *distance_item = new QListWidgetItem;
	distance_item->setData(0,distance);
	m_Controls->m_Distances->addItem(distance_item);
}

void QmitkDistanceCalculationBetweenPointsWidget::SetMultiWidget( QmitkStdMultiWidget* multiWidget )
{
  m_MultiWidget=multiWidget;
}

void QmitkDistanceCalculationBetweenPointsWidget::AddSliceNavigationController(mitk::SliceNavigationController* snc)
{
  m_Controls->m_ReferencePoints->AddSliceNavigationController(snc);
  m_Controls->m_MeasurementPoints->AddSliceNavigationController(snc);
}

mitk::DataNode::Pointer QmitkDistanceCalculationBetweenPointsWidget::GetReferencePositionNode()
{
  return m_ReferenceNode;
}

mitk::DataNode::Pointer QmitkDistanceCalculationBetweenPointsWidget::GetMeasurementPositionNode()
{
  return m_MeasurementNode;
}

void QmitkDistanceCalculationBetweenPointsWidget::AdjustButtonSpacing()
{
  if (m_Controls->m_AddMeasurementPositionBtn->isHidden() && m_Controls->m_AddReferencePositionBtn->isHidden())
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

void QmitkDistanceCalculationBetweenPointsWidget::SetReferenceLandmarkName(QString sourceLandmarkName)
{
  QString groupBoxTitle = sourceLandmarkName;
  groupBoxTitle.append(" Landmarks");
  m_Controls->ReferenceLandmarksGroupBox->setTitle(groupBoxTitle);
  QString buttonText = "Add ";
  buttonText.append(sourceLandmarkName);
  buttonText.append(" Landmark");
  m_Controls->m_AddReferencePositionBtn->setText(buttonText);
}

void QmitkDistanceCalculationBetweenPointsWidget::SetMeasurementLandmarkName(QString targetLandmarkName)
{
  QString groupBoxTitle = targetLandmarkName;
  groupBoxTitle.append(" Landmarks");
  m_Controls->MeasurementLandmarksGroupBox->setTitle(groupBoxTitle);
  QString buttonText = "Add ";
  buttonText.append(targetLandmarkName);
  buttonText.append(" Landmark");
  m_Controls->m_AddMeasurementPositionBtn->setText(buttonText);
}

void QmitkDistanceCalculationBetweenPointsWidget::OnSaveData()
{
	std::stringstream ss;
	std::string m_FileName;
	std::ostream* stream;

	//An existing extension will be cut and replaced with .xml
	std::string tmpPath = "C:/Users/lsanz";
	m_FileName = "Distances";
	std::string extension = ".xml";
	
	ss << tmpPath << "/" << m_FileName << extension;

	stream = new std::ofstream(ss.str().c_str());

	m_Stream = stream;
	m_Stream->precision(10);

	*m_Stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" << std::endl;
	*m_Stream << "    " << "<Data>" << std::endl;
	
	WriteData();

	*m_Stream << "</Data>" << std::endl;
	
	m_Stream->flush();
	m_Stream = NULL;
}

void QmitkDistanceCalculationBetweenPointsWidget::WriteData()
{
	
		// cast system time double value to stringstream to avoid low precision rounding
		std::ostringstream strs;
		strs.precision(15); // rounding precision for system time double value
		
		std::string sysTimeStr = strs.str();

		//write tool data for every tool
		mitk::PointSet::Pointer ReferencePointSet = dynamic_cast<mitk::PointSet*>(m_ReferenceNode->GetData());
		mitk::PointSet::Pointer MeasurementPointSet = dynamic_cast<mitk::PointSet*>(m_MeasurementNode->GetData());
		

		for (int index = 0; index<MeasurementPointSet->GetSize(); index++)
		{
			mitk::Point3D Ref_position = ReferencePointSet->GetPoint(0);
			mitk::Point3D Measurement_position = MeasurementPointSet->GetPoint(index);
			
			QString dist = m_Controls->m_Distances->item(index)->text();
			double distance = dist.toDouble();
		    TiXmlElement* elem = new TiXmlElement("Distance_between_points");

			elem->SetDoubleAttribute("Reference Position_X", Ref_position[0]);
			elem->SetDoubleAttribute("Reference Position_Y", Ref_position[1]);
			elem->SetDoubleAttribute("Reference Position_Z", Ref_position[2]);
			elem->SetDoubleAttribute("Measurement Position_X", Measurement_position[0]);
			elem->SetDoubleAttribute("Measurement Position_Y", Measurement_position[1]);
			elem->SetDoubleAttribute("Measurement Position_Z", Measurement_position[2]);
			elem->SetDoubleAttribute("Distance", distance);
			
			*m_Stream << "        " << *elem << std::endl;

			delete elem;
		}
	
}