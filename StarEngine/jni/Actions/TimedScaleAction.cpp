#include "TimedScaleAction.h"
#include "../Objects/Object.h"
#include "../Helpers/Math.h"

namespace star
{
	TimedScaleAction::TimedScaleAction(
		float32 seconds,
		float32 begin,
		float32 end,
		const std::function<void()> & callback
		)
		: TimedAction(seconds, callback)
		, m_BeginValue(begin)
		, m_EndValue(end)
		, m_CurrentSeconds(0)
	{
	}
	
	TimedScaleAction::TimedScaleAction(
		const tstring & name,
		float32 seconds,
		float32 begin,
		float32 end,
		const std::function<void()> & callback
		)
		: TimedAction(name, seconds, callback)
		, m_BeginValue(begin)
		, m_EndValue(end)
		, m_CurrentSeconds(0)
	{
	}
	
	TimedScaleAction::~TimedScaleAction()
	{	
	}
	
	void TimedScaleAction::Update(const Context & context)
	{
		m_CurrentSeconds += float32(context.mTimeManager->GetSeconds());
		
		m_pParent->GetTransform()->Scale
		(
			star::Lerp(m_BeginValue, m_EndValue, m_CurrentSeconds / m_Seconds)
		);
	}
	
	void TimedScaleAction::Restart()
	{
		m_CurrentSeconds = 0;
	}
}
