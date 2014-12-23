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

#include "QmitkMaxillofacialNavigationDataSourceSelectionWidget.h"

// MITK

#include <mitkNavigationDataSource.h>
#include <mitkTrackingDeviceSource.h>


//mitk headers
#include <usGetModuleContext.h>
#include <usServiceReference.h>
#include <usModuleInitialization.h>

QmitkMaxillofacialNavigationDataSourceSelectionWidget::QmitkMaxillofacialNavigationDataSourceSelectionWidget(QWidget* parent, Qt::WindowFlags f)
: QWidget(parent, f)
{
	m_Controls = NULL;
	CreateQtPartControl(this);
	CreateConnections();

}


QmitkMaxillofacialNavigationDataSourceSelectionWidget::~QmitkMaxillofacialNavigationDataSourceSelectionWidget()
{

}

void QmitkMaxillofacialNavigationDataSourceSelectionWidget::CreateQtPartControl(QWidget *parent)
{
	if (!m_Controls)
	{
		// create GUI widgets
		m_Controls = new Ui::QmitkMaxillofacialNavigationDataSourceSelectionWidgetControls;
		m_Controls->setupUi(parent);

		std::string empty = "";
		m_Controls->m_NavigationDataSourceWidget->Initialize<mitk::NavigationDataSource>(mitk::NavigationDataSource::US_PROPKEY_DEVICENAME, empty);

	}
}

void QmitkMaxillofacialNavigationDataSourceSelectionWidget::CreateConnections()
{
	if (m_Controls)
	{
		connect((QObject*)(m_Controls->m_NavigationDataSourceWidget), SIGNAL(ServiceSelectionChanged(us::ServiceReferenceU)), this, SLOT(NavigationDataSourceSelected(us::ServiceReferenceU)));
		connect((QObject*)(m_Controls->m_ToolView), SIGNAL(itemSelectionChanged()), this, SIGNAL(NavigationToolSelected()));
		//connect((QObject*)(m_Controls->m_ToolView), SIGNAL(itemSelectionChanged()), this, SLOT(NavigationToolSelected()));
		
	}
}


void QmitkMaxillofacialNavigationDataSourceSelectionWidget::NavigationDataSourceSelected(us::ServiceReferenceU s)
{
	if (!s) //no device selected
	{
		//reset everything
		m_CurrentSource = NULL;
		m_CurrentStorage = NULL;
		emit NavigationDataSourceSelected(m_CurrentSource);
		return;
	}

	// Get Source
	us::ModuleContext* context = us::GetModuleContext();
	m_CurrentSource = context->GetService<mitk::NavigationDataSource>(s);
	std::string id = s.GetProperty(mitk::NavigationDataSource::US_PROPKEY_ID).ToString();
	std::cout << "DataSource ID: " << id << std::endl;

	// clear tool list before filling it
	m_Controls->m_ToolView->clear();
	//Fill tool list
	MITK_INFO << "no outputs: " << m_CurrentSource->GetNumberOfOutputs();
	for (std::size_t i = 0; i < m_CurrentSource->GetNumberOfOutputs(); i++)
	{
		new QListWidgetItem(tr(m_CurrentSource->GetOutput(i)->GetName()), m_Controls->m_ToolView);
	}


	// Create Filter for ToolStorage
	std::string filter = "(" + mitk::NavigationToolStorage::US_PROPKEY_SOURCE_ID + "=" + id + ")";

	// Get Storage
	std::vector<us::ServiceReference<mitk::NavigationToolStorage> > refs = context->GetServiceReferences<mitk::NavigationToolStorage>(filter);
	if (refs.empty()) return; //no storage was found
	m_CurrentStorage = context->GetService(refs.front());
	if (m_CurrentStorage.IsNull())
	{
		MITK_WARN << "Found an invalid storage object!";
		return;
	}
	if (m_CurrentStorage->GetToolCount() != m_CurrentSource->GetNumberOfOutputs()) //there is something wrong with the storage
	{
		MITK_WARN << "Found a tool storage, but it has not the same number of tools like the NavigationDataSource. This storage won't be used because it isn't the right one.";
		m_CurrentStorage = NULL;
	}

	emit NavigationDataSourceSelected(m_CurrentSource);
}

/*void QmitkMaxillofacialNavigationDataSourceSelectionWidget::NavigationToolSelected()
{
	emit NavigationToolSelected();
}*/

mitk::NavigationDataSource::Pointer QmitkMaxillofacialNavigationDataSourceSelectionWidget::GetSelectedNavigationDataSource()
{
	return this->m_CurrentSource;
}


int QmitkMaxillofacialNavigationDataSourceSelectionWidget::GetSelectedToolID()
{
	return this->m_Controls->m_ToolView->currentIndex().row();
}


mitk::NavigationTool::Pointer QmitkMaxillofacialNavigationDataSourceSelectionWidget::GetSelectedNavigationTool()
{
	if (this->m_CurrentStorage.IsNull()) return NULL;
	if (m_Controls->m_ToolView->currentIndex().row() >= m_CurrentStorage->GetToolCount()) return NULL;
	return this->m_CurrentStorage->GetTool(m_Controls->m_ToolView->currentIndex().row());
}


mitk::NavigationToolStorage::Pointer QmitkMaxillofacialNavigationDataSourceSelectionWidget::GetNavigationToolStorageOfSource()
{
	return this->m_CurrentStorage;
}
