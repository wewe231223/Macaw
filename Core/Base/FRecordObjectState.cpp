#include "PCH.h"
#include "FRecordObjectState.h"

#include "ObjectSystem.h" 
#include "UObject.h"   

void FRecordObjectState::ApplyUndo() {
    UObject* TargetObject = ObjectSystem::Resolve(ObjectSystem::FindHandleByGuid(TargetGuid));
    if (TargetObject == nullptr)
        return;


    // test
    TargetObject->Data = BeforeData[0];

    // TODO
    // 1. BeforeData를 이용해 TargetObject의 상태를 뒤로 돌림 (직렬화 로직)
    // TargetObject->Deserialize(BeforeData);

    // 2. 이벤트 채널을 통해 엔진 전역에 "객체가 변경됨!"을 알림
    // FEventChannel::Broadcast(EEngineEvent::ObjectModified, TargetObject);
}

void FRecordObjectState::ApplyRedo() {
    UObject* TargetObject = ObjectSystem::Resolve(ObjectSystem::FindHandleByGuid(TargetGuid));
    if (TargetObject == nullptr)
        return;

    // test
    TargetObject->Data = AfterData[0];

    // TODO
    // 1. AfterData를 이용해 TargetObject의 상태를 앞으로 돌림 (직렬화 로직)
    // TargetObject->Deserialize(AfterData);

    // 2. 이벤트 채널을 통해 엔진 전역에 "객체가 변경됨!"을 알림
    // FEventChannel::Broadcast(EEngineEvent::ObjectModified, TargetObject);
}