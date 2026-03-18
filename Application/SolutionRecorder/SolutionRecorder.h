#pragma once

struct SolutionStep
{
	UINT fromID;
	UINT toID;
};

class SolutionRecorder
{
public:
	void StartRecrding()
	{
		//記録開始時に既存の手順をクリア
		m_solutionSteps.clear();
		m_isRecording = true;
	}

	void StopRecording()
	{
		m_isRecording = false;
	}

	bool IsRecording()const { return m_isRecording; }

	void AddStep(UINT fromID, UINT toID)
	{
		//模範解答の作成中のみ、プレイヤーの操作手順(どのブロックとどのブロックを入れ替えたか)を記録する
		if (m_isRecording)
		{
			m_solutionSteps.push_back({ fromID,toID });
		}
	}

	void Clear()
	{
		m_solutionSteps.clear();
	}

	const std::vector<SolutionStep>& GetSolutionSteps()const
	{
		return m_solutionSteps;
	}

private:
	bool m_isRecording = false;
	std::vector<SolutionStep> m_solutionSteps;
};