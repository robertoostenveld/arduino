/*
   See https://www.johndcook.com/blog/standard_deviation/ and https://www.johndcook.com/blog/skewness_kurtosis/
*/

#ifndef _RUNNINGSTAT_H_
#define _RUNNINGSTAT_H_

class RunningStat
{
  public:
    RunningStat() : m_n(0) {}

    void Clear()
    {
      m_n = 0;
      m_prev = 0;
      m_crossing = 0;
    }

    void Push(double x)
    {
      m_n++;

      // see https://en.wikipedia.org/wiki/Zero-crossing_rate
      m_crossing += ((x > 0) ^ (m_prev > 0)); // bitwise xor
      m_prev = x;

      // See Knuth TAOCP vol 2, 3rd edition, page 232
      if (m_n == 1)
      {
        m_oldM = m_newM = x;
        m_oldS = 0.0;
      }
      else
      {
        m_newM = m_oldM + (x - m_oldM) / m_n;
        m_newS = m_oldS + (x - m_oldM) * (x - m_newM);

        // set up for next iteration
        m_oldM = m_newM;
        m_oldS = m_newS;
      }

      if (m_n == 1) {
        m_min = x;
        m_max = x;
      }
      else {
        m_min = (x < m_min ? x : m_min);
        m_max = (x > m_max ? x : m_max);
      }
    }

    int NumDataValues() const
    {
      return m_n;
    }

    double Mean() const
    {
      return (m_n > 0) ? m_newM : 0.0;
    }

    double Variance() const
    {
      return ( (m_n > 1) ? m_newS / (m_n - 1) : 0.0 );
    }

    double StandardDeviation() const
    {
      return sqrt( Variance() );
    }

    double Min() const
    {
      return m_min;
    }

    double Max() const
    {
      return m_max;
    }

    double ZeroCrossingRate() const
    {
      return double(m_crossing) / double(m_n) / 2;
    }

  private:
    int m_n, m_crossing;
    double m_oldM, m_newM, m_oldS, m_newS, m_min, m_max, m_prev;
};

#endif _RUNNINGSTAT_H_
