#pragma once

#include <Siv3D.hpp>

namespace tomolatoon
{

	struct LerpTransition
	{
		using self = LerpTransition;

		LerpTransition(const Duration& inDuration, const Duration& outDuration, double start, double finish, Optional<double> init = none)
			: m_transition(inDuration, outDuration, (finish - start != 0) ? ((init.value_or(start) - start) / (finish - start)) : (0.0))
			, m_start(start)
			, m_finish(finish)
			, m_prevTransition(m_transition.value())
			, m_inTime(inDuration.count())
			, m_outTime(outDuration.count())
		{}

		double getStart() const
		{
			return m_start;
		}

		double getFinish() const
		{
			return m_finish;
		}

		// 構造化束縛して使う
		auto getRange() const
		{
			return std::pair(m_start, m_finish);
		}

		self& setRange(Optional<double> start, Optional<double> finish)
		{
			m_start  = start.value_or(m_start);
			m_finish = finish.value_or(m_finish);

			return *this;
		}

		self& setRange(std::pair<double, double> pair)
		{
			m_start  = pair.first;
			m_finish = pair.second;

			return *this;
		}

		void updateByDeltaSec(bool in, double deltaSec = Scene::DeltaTime())
		{
			m_prevTransition = m_transition.value();
			m_transition.update(in, deltaSec);
		}

		bool isFinish() const
		{
			return m_transition.isOne();
		}

		bool isStart() const
		{
			return m_transition.isZero();
		}

		double value(double easeFunc(double) = Easing::Linear) const
		{
			return Math::Lerp(m_start, m_finish, easeFunc(m_transition.value()));
		}

		double deltaValue(double easeFunc(double) = Easing::Linear) const
		{
			return value(easeFunc) - Math::Lerp(m_start, m_finish, easeFunc(m_prevTransition));
		}

		double transition(double easeFunc(double) = Easing::Linear) const
		{
			return easeFunc(m_transition.value());
		}

		double deltaTransition(double easeFunc(double) = Easing::Linear) const
		{
			return easeFunc(m_transition.value()) - easeFunc(m_prevTransition);
		}

		double setFinish(double thresholdPercent = 1.0)
		{
			if (1.0 - value() <= thresholdPercent)
			{
				m_transition.update(true, m_inTime + 1.0);
			}

			return m_finish;
		}

		double setStart(double thresholdPercent = 1.0)
		{
			if (1.0 - value() <= thresholdPercent)
			{
				m_transition.update(false, m_outTime + 1.0);
			}

			return m_start;
		}

	private:
		Transition m_transition;
		double     m_start;
		double     m_finish;
		double     m_prevTransition;
		double     m_inTime;
		double     m_outTime;
	};
} // namespace tomolatoon
