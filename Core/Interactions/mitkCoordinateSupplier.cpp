#include "mitkDisplayCoordinateOperation.h"
//has to be on top, otherwise compiler error!
#include "mitkCoordinateSupplier.h"
#include "Operation.h"
#include "OperationActor.h"
#include "mitkPointOperation.h"
#include "PositionEvent.h"
//and not here!
#include <string>
#include "mitkInteractionConst.h"


//##ModelId=3F0189F0025B
mitk::CoordinateSupplier::CoordinateSupplier(std::string type, mitk::OperationActor* operationActor)
: mitk::StateMachine(type), m_Destination(operationActor)
{
}

//##ModelId=3F0189F00269
bool mitk::CoordinateSupplier::ExecuteSideEffect(int sideEffectId, mitk::StateEvent const* stateEvent, int objectEventId, int groupEventId)
{
    bool ok = false;
    if (m_Destination == NULL)
        return false;
	
    const PositionEvent* posEvent = dynamic_cast<const PositionEvent*>(stateEvent->GetEvent());
    if(posEvent!=NULL)
    {
      switch (sideEffectId)
      {
        case SeNEWPOINT:
        {
			    mitk::Point3D newPoint = posEvent->GetWorldPosition();
          vm2itk(newPoint, m_OldPoint);

			    PointOperation* doOp = new mitk::PointOperation(OpADD, m_OldPoint, 0);
			    //Undo
          if (m_UndoEnabled)
          {
				    PointOperation* undoOp = new PointOperation(OpDELETE, m_OldPoint, 0);
            OperationEvent *operationEvent = new OperationEvent(m_Destination, doOp, undoOp,
						                                      					    objectEventId, groupEventId);
            m_UndoController->SetOperationEvent(operationEvent);
          }
          //execute the Operation
			    m_Destination->ExecuteOperation(doOp);
          ok = true;
          break;
        }
        case SeINITMOVEMENT:
        {//move the point to the coordinate //not used, cause same to MovePoint... check xml-file
          mitk::ITKPoint3D movePoint;
          vm2itk(posEvent->GetWorldPosition(), movePoint);

          PointOperation* doOp = new mitk::PointOperation(OpMOVE, movePoint, 0);
          //execute the Operation
			    m_Destination->ExecuteOperation(doOp);
          ok = true;
          break;
        }
        case SeMOVEPOINT:
        {
          mitk::ITKPoint3D movePoint;
          vm2itk(posEvent->GetWorldPosition(), movePoint);

          PointOperation* doOp = new mitk::PointOperation(OpMOVE, movePoint, 0);
          //execute the Operation
			    m_Destination->ExecuteOperation(doOp);
          ok = true;
          break;
        }
        case SeFINISHMOVEMENT:
        {
          mitk::ITKPoint3D movePoint, oldMovePoint;
          oldMovePoint.Fill(0);
          vm2itk(posEvent->GetWorldPosition(), movePoint);
          PointOperation* doOp = new mitk::PointOperation(OpMOVE, movePoint, 0);
          if (m_UndoEnabled )//&& (posEvent->GetType() == mitk::Type_MouseButtonRelease)
          {
            //get the last Position from the UndoList
            OperationEvent *lastOperationEvent = m_UndoController->GetLastOfType(m_Destination, OpMOVE);
            if (lastOperationEvent != NULL)
            {
              PointOperation* lastOp = dynamic_cast<PointOperation *>(lastOperationEvent->GetOperation());
              if (lastOp != NULL)
              {
                oldMovePoint = lastOp->GetPoint();
              }
            }
            PointOperation* undoOp = new PointOperation(OpMOVE, oldMovePoint, 0);
            OperationEvent *operationEvent = new OperationEvent(m_Destination, doOp, undoOp,
						                                                    objectEventId, groupEventId);
            m_UndoController->SetOperationEvent(operationEvent);
          }
          //execute the Operation
	  		  m_Destination->ExecuteOperation(doOp);
          ok = true;
          break;
        }
        default:
          ok = false;
          break;
        }
       return ok;
    }
    
    const mitk::DisplayPositionEvent* displPosEvent = dynamic_cast<const mitk::DisplayPositionEvent *>(stateEvent->GetEvent());
    if(displPosEvent!=NULL)
    {
        return true;
    }

	return false;
}
