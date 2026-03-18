#include "Pch.h"
#include "SlipperyComponent.h"
#include"../GameObject.h"
#include"../RigidbodyComponent/RigidbodyComponent.h"
#include"../Src/Application/main.h"

void SlipperyComponent::OnInspect()
{
	if (ImGui::CollapsingHeader("Slippery Component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		//抵抗係数(0に近いほど滑る)
		ImGui::DragFloat("Drag Coefficient", &m_dragCoefficient, 0.01f, 0.0f, 2.0f);
	}
}