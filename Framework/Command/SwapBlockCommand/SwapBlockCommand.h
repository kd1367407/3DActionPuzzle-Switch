#pragma once
#include"../Command.h"

class StageModel;

class SwapBlockCommand :public ICommand
{
public:
	SwapBlockCommand(std::shared_ptr<StageModel> model, unsigned int id1, unsigned int id2, bool isEditorRequest = false);

	void Execute()override;
	void Undo()override;


private:
	std::weak_ptr<StageModel> m_wpModel;
	unsigned int m_id1, m_id2;
	bool m_isEditorRequest;
};