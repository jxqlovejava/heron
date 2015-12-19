/*
 * Copyright 2015 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//////////////////////////////////////////////////////////////////////////////
//
// time-spent-metric.cpp
//
// Please see time-spent-metric.h for details
//////////////////////////////////////////////////////////////////////////////
#include "proto/messages.h"
#include "basics/basics.h"
#include "errors/errors.h"
#include "threads/threads.h"
#include "network/network.h"

#include "metrics/imetric.h"
#include "metrics/time-spent-metric.h"

#include <sstream>

namespace heron { namespace common {

TimeSpentMetric::TimeSpentMetric()
  : total_time_msecs_(0), started_(false)
{
}

TimeSpentMetric::~TimeSpentMetric()
{
}

void TimeSpentMetric::Start()
{
  using namespace std::chrono;

  if (!started_) {
    start_time_ = high_resolution_clock::now();
    started_ = true;
  }
  // If it is already started, just ignore.
}

void TimeSpentMetric::Stop()
{
  using namespace std::chrono;

  if (started_) {
    auto end_time = high_resolution_clock::now();
    total_time_msecs_ += duration_cast<milliseconds>(end_time - start_time_).count();
    start_time_ = high_resolution_clock::now();
    started_ = false;
  }
  // If it is already stopped, just ignore.
}

void 
TimeSpentMetric::GetAndReset(
  const sp_string&                              _prefix,
  proto::system::MetricPublisherPublishMessage* _message) 
{
  using namespace std::chrono;

  if (started_) {
    auto now = high_resolution_clock::now();
    total_time_msecs_ += duration_cast<milliseconds>(now - start_time_).count();
    start_time_ = now;
  }
  std::ostringstream o;
  o << total_time_msecs_;
  total_time_msecs_ = 0;
  proto::system::MetricDatum* d = _message->add_metrics();
  d->set_name(_prefix);
  d->set_value(o.str());
}

}} // end namespace
