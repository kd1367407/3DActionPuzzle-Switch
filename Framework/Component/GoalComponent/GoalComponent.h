#pragma once
#include"../Component.h"
#include"../ITriggerReceiver.h"

class GoalComponent:public Component,public ITriggerReceiver
{
public:

    void Awake()override;
    //ITriggerReceiver‚ÌŽÀ‘•
    void OnTriggerEnter(const CollisionInfo& info) override;

    const char* GetComponentName() const override { return "GoalComponent"; }

private:

};