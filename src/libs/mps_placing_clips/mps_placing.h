/***************************************************************************
 *  mps_placing_clips.h - mps_placing generator for CLIPS
 *  Created: Fri May 5 09:28:09 2017
 *  Copyright  2017 Gerald Steinbauer <steinbauer@ist.tugraz.at>
 *             2017 Tobias Neumann <t.neumann@fh-aachen.de>
 *             2019 Thomas Ulz <thomas.ulz@tugraz.at>
 ****************************************************************************/

/*  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the authors nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MPS_PLACING_H
#define MPS_PLACING_H

#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/search.hh>
#include <set>
#include <vector>

#define EMPTY_ROT 0
#define ANGLE_0 1
#define ANGLE_45 2
#define ANGLE_90 3
#define ANGLE_135 4
#define ANGLE_180 5
#define ANGLE_225 6
#define ANGLE_270 7
#define ANGLE_315 8

#define EMPTY_ROT 0
#define BASE 1
#define CAP1 2
#define CAP2 3
#define RING1 4
#define RING2 5
#define STORAGE 6
#define DELIVERY 7
#define NUM_MPS 7
#define ALL_MPS BASE, CAP1, CAP2, RING1, RING2, STORAGE, DELIVERY

#define TIMEOUT_MS 3000

class MPSPlacingPlacing
{
public:
	MPSPlacingPlacing(int x, int y, int type, int angle)
	{
		x_     = x;
		y_     = y;
		type_  = type;
		angle_ = angle;
	}

	int x_;
	int y_;
	int type_;
	int angle_;
};

class MPSPlacing : public Gecode::IntMinimizeSpace
{
public:
	MPSPlacing(int _width, int _height, std::set<int> _machines)
	{
		height_   = _height;
		width_    = _width;
		machines_ = _machines;

		mps_type_  = Gecode::IntVarArray(*this, (height_ + 2) * (width_ + 2), EMPTY_ROT, NUM_MPS);
		mps_angle_ = Gecode::IntVarArray(*this, (height_ + 2) * (width_ + 2), EMPTY_ROT, ANGLE_315);

		rg_ = Gecode::Rnd(time(NULL));

		std::vector<int> types({EMPTY_ROT, ALL_MPS});
		mps_types_ = Gecode::IntArgs(types);
		mps_count_ = Gecode::IntVarArray(*this, NUM_MPS + 1, EMPTY_ROT, (height_ + 2) * (width_ + 2));

		// mps_resource are reserved zones needed to operate mps
		mps_resource_.resize(width_ + 2);
		for (int x = 0; x < width_ + 2; x++) {
			mps_resource_[x].resize(height_ + 2);
			for (int y = 0; y < height_ + 2; y++) {
				mps_resource_[x][y] = Gecode::IntVarArray(*this, NUM_MPS, 0, 1);
			}
		}

		// counting constraint for number of different machines
		for (const auto &mps : {ALL_MPS}) {
			if (machines_.find(mps) != machines_.end()) {
				Gecode::rel(*this, mps_count_[mps], Gecode::IRT_EQ, 1);
			} else {
				Gecode::rel(*this, mps_count_[mps], Gecode::IRT_EQ, 0);
			}
		}

		// an EMPTY_ROT zone has no orientation
		for (int i = 0; i < (height_ + 2) * (width_ + 2); i++) {
			Gecode::rel(*this, (mps_type_[i] == EMPTY_ROT) >> (mps_angle_[i] == EMPTY_ROT));
		}

		// placed machine must have an orientation
		for (int i = 0; i < (height_ + 2) * (width_ + 2); i++) {
			Gecode::rel(*this, (mps_type_[i] != EMPTY_ROT) >> (mps_angle_[i] != EMPTY_ROT));
		}

		// mark the border blocked
		for (int x = 0; x < width_ + 2; x++) {
			Gecode::rel(*this, mps_type_[index(x, 0)] == EMPTY_ROT);
			Gecode::rel(*this, mps_resource_[x][0][0] == 1);
			Gecode::rel(*this, mps_type_[index(x, height_ + 1)] == EMPTY_ROT);
			Gecode::rel(*this, mps_resource_[x][height_ + 1][0] == 1);
		}
		for (int y = 0; y < height_ + 2; y++) {
			Gecode::rel(*this, mps_type_[index(0, y)] == EMPTY_ROT);
			Gecode::rel(*this, mps_resource_[0][y][0] == 1);
			Gecode::rel(*this, mps_type_[index(width_ + 1, y)] == EMPTY_ROT);
			Gecode::rel(*this, mps_resource_[width_ + 1][y][BASE - 1] == 1);
		}

		// machines where both sides are usable and no additional constraints hold
		std::vector<int> symmetric_mps;
		std::vector<int> mps_group = {BASE, STORAGE};
		std::sort(mps_group.begin(), mps_group.end());
		set_intersection(mps_group.begin(),
		                 mps_group.end(),
		                 machines_.begin(),
		                 machines_.end(),
		                 std::back_inserter(symmetric_mps));
		// machines where the left side of input must not be close to a border
		std::vector<int> asymmetric_mps;
		mps_group = {CAP1, CAP2, RING1, RING2};
		std::sort(mps_group.begin(), mps_group.end());
		set_intersection(mps_group.begin(),
		                 mps_group.end(),
		                 machines_.begin(),
		                 machines_.end(),
		                 std::back_inserter(asymmetric_mps));
		// machines where only the input side is used
		std::vector<int> input_only_mps;
		;
		mps_group = {DELIVERY};
		std::sort(mps_group.begin(), mps_group.end());
		set_intersection(mps_group.begin(),
		                 mps_group.end(),
		                 machines_.begin(),
		                 machines_.end(),
		                 std::back_inserter(input_only_mps));

		// along x border
		for (int x = 1; x <= width_; x++) {
			for (int t : symmetric_mps) {
				Gecode::rel(*this,
				            (mps_type_[index(x, 1)] == t) >> ((mps_angle_[index(x, 1)] == ANGLE_0)
				                                              || (mps_angle_[index(x, 1)] == ANGLE_180)));
				Gecode::rel(*this,
				            (mps_type_[index(x, height_)] == t)
				              >> ((mps_angle_[index(x, height_)] == ANGLE_0)
				                  || (mps_angle_[index(x, height_)] == ANGLE_180)));
			}
			for (int t : asymmetric_mps) {
				Gecode::rel(*this, (mps_type_[index(x, 1)] == t) >> ((mps_angle_[index(x, 1)] == ANGLE_0)));
				Gecode::rel(*this,
				            (mps_type_[index(x, height_)] == t)
				              >> ((mps_angle_[index(x, height_)] == ANGLE_180)));
			}
			// machines that only use the input side
			for (int t : input_only_mps) {
				Gecode::rel(*this,
				            (mps_type_[index(x, 1)] == t) >> ((mps_angle_[index(x, 1)] != ANGLE_225)
				                                              && (mps_angle_[index(x, 1)] != ANGLE_270)
				                                              && (mps_angle_[index(x, 1)] != ANGLE_315)));
				Gecode::rel(*this,
				            (mps_type_[index(x, height_)] == t)
				              >> ((mps_angle_[index(x, 1)] != ANGLE_45)
				                  && (mps_angle_[index(x, 1)] != ANGLE_90)
				                  && (mps_angle_[index(x, 1)] != ANGLE_135)));
			}
		}

		// special case entry zone wall
		for (int x = width_ - 1; x <= width_; x++) {
			for (int t : symmetric_mps) {
				Gecode::rel(*this,
				            (mps_type_[index(x, 2)] == t) >> ((mps_angle_[index(x, 1)] == ANGLE_0)
				                                              || (mps_angle_[index(x, 1)] == ANGLE_180)));
			}
			for (int t : asymmetric_mps) {
				Gecode::rel(*this, (mps_type_[index(x, 2)] == t) >> ((mps_angle_[index(x, 1)] == ANGLE_0)));
			}
			for (int t : input_only_mps) {
				Gecode::rel(*this,
				            (mps_type_[index(x, 2)] == t) >> ((mps_angle_[index(x, 1)] != ANGLE_225)
				                                              && (mps_angle_[index(x, 1)] != ANGLE_270)
				                                              && (mps_angle_[index(x, 1)] != ANGLE_315)));
			}
		}

		// along y border
		for (int y = 1; y <= height_; y++) {
			// BASE
			for (int t : symmetric_mps) {
				Gecode::rel(*this,
				            (mps_type_[index(1, y)] == t) >> ((mps_angle_[index(1, y)] == ANGLE_90)
				                                              || (mps_angle_[index(1, y)] == ANGLE_270)));
				Gecode::rel(*this,
				            (mps_type_[index(width_, y)] == t)
				              >> ((mps_angle_[index(width_, y)] == ANGLE_90)
				                  || (mps_angle_[index(width_, y)] == ANGLE_270)));
			}
			for (int t : asymmetric_mps) {
				// left side is open due to symmetry of field
				Gecode::rel(*this,
				            (mps_type_[index(1, y)] == t) >> ((mps_angle_[index(1, y)] == ANGLE_90)
				                                              || (mps_angle_[index(1, y)] == ANGLE_270)));
				Gecode::rel(*this,
				            (mps_type_[index(width_, y)] == t)
				              >> ((mps_angle_[index(width_, y)] == ANGLE_270)));
			}
			for (int t : input_only_mps) {
				Gecode::rel(*this,
				            (mps_type_[index(1, y)] == t) >> ((mps_angle_[index(1, y)] != ANGLE_45)
				                                              && (mps_angle_[index(1, y)] != ANGLE_0)
				                                              && (mps_angle_[index(1, y)] != ANGLE_315)));
				Gecode::rel(*this,
				            (mps_type_[index(width_, y)] == t)
				              >> ((mps_angle_[index(width_, y)] != ANGLE_135)
				                  && (mps_angle_[index(width_, y)] != ANGLE_180)
				                  && (mps_angle_[index(width_, y)] != ANGLE_225)));
			}
		}
		//special case entry wall
		// no machine with 2 sides can be placed next to the entry wall, hence
		// only restriction for one-sided machines are needed
		for (int t : input_only_mps) {
			Gecode::rel(*this,
			            (mps_type_[index(width_ - 3, 1)] == t)
			              >> ((mps_angle_[index(width_ - 3, 1)] != ANGLE_135)
			                  && (mps_angle_[index(width_ - 3, 1)] != ANGLE_180)
			                  && (mps_angle_[index(width_ - 3, 1)] != ANGLE_225)));
		}

		// entry zone is blocked
		Gecode::rel(*this, mps_resource_[width_][1][0] == 1);
		Gecode::rel(*this, mps_resource_[width_ - 1][1][0] == 1);
		Gecode::rel(*this, mps_resource_[width_ - 2][1][0] == 1);
		Gecode::rel(*this, mps_type_[index(width_, 1)] == EMPTY_ROT);
		Gecode::rel(*this, mps_type_[index(width_ - 1, 1)] == EMPTY_ROT);
		Gecode::rel(*this, mps_type_[index(width_ - 2, 1)] == EMPTY_ROT);
		Gecode::rel(*this, mps_type_[index(width_ - 2, 2)] == EMPTY_ROT);

		// insert blocking constraints
		for (int x = 1; x <= width_; x++) {
			for (int y = 1; y <= height_; y++) {
				for (int t : machines_) {
					for (int angle : {ANGLE_0,
					                  ANGLE_45,
					                  ANGLE_90,
					                  ANGLE_135,
					                  ANGLE_180,
					                  ANGLE_225,
					                  ANGLE_270,
					                  ANGLE_315}) {
						//reserve the station itself
						Gecode::rel(*this,
						            ((mps_angle_[index(x, y)] == angle) && (mps_type_[index(x, y)] == t))
						              >> (mps_resource_[x][y][t - 1] == 1));
						// reserve top and bottom zones
						if (angle != ANGLE_0 && angle != ANGLE_180) {
							if (std::find(input_only_mps.begin(), input_only_mps.end(), t) == input_only_mps.end()
							    || angle > ANGLE_180) {
								Gecode::rel(*this,
								            ((mps_angle_[index(x, y)] == angle) && (mps_type_[index(x, y)] == t))
								              >> (mps_resource_[x][y - 1][t - 1] == 1));
							}
							if (std::find(input_only_mps.begin(), input_only_mps.end(), t) == input_only_mps.end()
							    || angle < ANGLE_180) {
								Gecode::rel(*this,
								            ((mps_angle_[index(x, y)] == angle) && (mps_type_[index(x, y)] == t))
								              >> (mps_resource_[x][y + 1][t - 1] == 1));
							}
						}
						// reserve left and right zone
						if (angle != ANGLE_90 && angle != ANGLE_270) {
							if (std::find(input_only_mps.begin(), input_only_mps.end(), t) == input_only_mps.end()
							    || angle < ANGLE_90 || angle > ANGLE_270) {
								Gecode::rel(*this,
								            ((mps_angle_[index(x, y)] == angle) && (mps_type_[index(x, y)] == t))
								              >> (mps_resource_[x + 1][y][t - 1] == 1));
							}
							if (std::find(input_only_mps.begin(), input_only_mps.end(), t) == input_only_mps.end()
							    || (angle > ANGLE_90 && angle < ANGLE_270)) {
								Gecode::rel(*this,
								            ((mps_angle_[index(x, y)] == angle) && (mps_type_[index(x, y)] == t))
								              >> (mps_resource_[x - 1][y][t - 1] == 1));
							}
						}
					}
					// reserve top right and bottom left zone
					for (int angle : {ANGLE_45, ANGLE_225}) {
						if (std::find(input_only_mps.begin(), input_only_mps.end(), t) == input_only_mps.end()
						    || angle != ANGLE_225) {
							Gecode::rel(*this,
							            ((mps_angle_[index(x, y)] == angle) && (mps_type_[index(x, y)] == t))
							              >> (mps_resource_[x + 1][y + 1][t - 1] == 1));
						}
						if (std::find(input_only_mps.begin(), input_only_mps.end(), t) == input_only_mps.end()
						    || angle != ANGLE_45) {
							Gecode::rel(*this,
							            ((mps_angle_[index(x, y)] == angle) && (mps_type_[index(x, y)] == t))
							              >> (mps_resource_[x - 1][y - 1][t - 1] == 1));
						}
					}
					// reserve top left and bottom right zone
					for (int angle : {ANGLE_135, ANGLE_315}) {
						if (std::find(input_only_mps.begin(), input_only_mps.end(), t) == input_only_mps.end()
						    || angle != ANGLE_135) {
							Gecode::rel(*this,
							            ((mps_angle_[index(x, y)] == angle) && (mps_type_[index(x, y)] == t))
							              >> (mps_resource_[x - 1][y + 1][t - 1] == 1));
						}
						if (std::find(input_only_mps.begin(), input_only_mps.end(), t) == input_only_mps.end()
						    || angle != ANGLE_315) {
							Gecode::rel(*this,
							            ((mps_angle_[index(x, y)] == angle) && (mps_type_[index(x, y)] == t))
							              >> (mps_resource_[x + 1][y - 1][t - 1] == 1));
						}
					}
				}
			}
		}

		// prevent more than 2 machines in a line

		//vertical
		for (int x = 1; x <= width_; x++) {
			for (int y = 1; y <= height_ - 2; y++) {
				Gecode::rel(*this,
				            ((mps_type_[index(x, y)] != EMPTY_ROT)
				             && (mps_type_[index(x, y + 1)] != EMPTY_ROT))
				              >> (mps_type_[index(x, y + 2)] == EMPTY_ROT));
			}
		}

		//horizontal
		for (int y = 1; y <= height_; y++) {
			for (int x = 1; x <= width_ - 2; x++) {
				Gecode::rel(*this,
				            ((mps_type_[index(x, y)] != EMPTY_ROT)
				             && (mps_type_[index(x + 1, y)] != EMPTY_ROT))
				              >> (mps_type_[index(x + 2, y)] == EMPTY_ROT));
			}
		}

		//diagonal
		for (int x = 1; x <= width_ - 2; x++) {
			for (int y = 1; y <= height_ - 2; y++) {
				Gecode::rel(*this,
				            ((mps_type_[index(x, y)] != EMPTY_ROT)
				             && (mps_type_[index(x + 1, y + 1)] != EMPTY_ROT))
				              >> (mps_type_[index(x + 2, y + 2)] == EMPTY_ROT));
			}
		}
		for (int x = 3; x <= width_; x++) {
			for (int y = 1; y <= height_ - 2; y++) {
				Gecode::rel(*this,
				            ((mps_type_[index(x, y)] != EMPTY_ROT)
				             && (mps_type_[index(x - 1, y + 1)] != EMPTY_ROT))
				              >> (mps_type_[index(x - 2, y + 2)] == EMPTY_ROT));
			}
		}

		// avoid locks in corners
		Gecode::rel(*this,
		            (mps_type_[index(2, 1)] != EMPTY_ROT) >> (mps_type_[index(1, 2)] == EMPTY_ROT));
		Gecode::rel(*this,
		            (mps_type_[index(1, 2)] != EMPTY_ROT) >> (mps_type_[index(2, 1)] == EMPTY_ROT));

		Gecode::rel(*this,
		            (mps_type_[index(2, height_)] != EMPTY_ROT)
		              >> (mps_type_[index(1, height_ - 1)] == EMPTY_ROT));
		Gecode::rel(*this,
		            (mps_type_[index(1, height_ - 1)] != EMPTY_ROT)
		              >> (mps_type_[index(2, height_)] == EMPTY_ROT));

		Gecode::rel(*this,
		            (mps_type_[index(width_ - 1, height_)] != EMPTY_ROT)
		              >> (mps_type_[index(width_, height_ - 1)] == EMPTY_ROT));
		Gecode::rel(*this,
		            (mps_type_[index(width_, height_ - 1)] != EMPTY_ROT)
		              >> (mps_type_[index(width_ - 1, height_)] == EMPTY_ROT));

		Gecode::rel(*this,
		            (mps_type_[index(width_ - 1, 2)] != EMPTY_ROT)
		              >> (mps_type_[index(width_, 3)] == EMPTY_ROT));
		Gecode::rel(*this,
		            (mps_type_[index(width_, 3)] != EMPTY_ROT)
		              >> (mps_type_[index(width_ - 1, 2)] == EMPTY_ROT));

		Gecode::rel(*this,
		            (mps_type_[index(1, 1)] != EMPTY_ROT) >> (mps_angle_[index(1, 1)] == ANGLE_135));
		Gecode::rel(*this,
		            (mps_type_[index(1, height_)] != EMPTY_ROT)
		              >> (mps_angle_[index(1, height_)] == ANGLE_315));
		Gecode::rel(*this,
		            (mps_type_[index(width_, height_)] != EMPTY_ROT)
		              >> (mps_angle_[index(width_, height_)] == ANGLE_135));
		Gecode::rel(*this,
		            (mps_type_[index(width_, 2)] != EMPTY_ROT)
		              >> (mps_angle_[index(width_, 2)] == ANGLE_45));
		Gecode::rel(*this,
		            (mps_type_[index(width_ - 3, 1)] != EMPTY_ROT)
		              >> (mps_angle_[index(width_ - 3, 1)] == ANGLE_45));

		// restrict zone usage to max 1
		for (int x = 0; x < width_ + 2; x++) {
			for (int y = 0; y < height_ + 2; y++) {
				Gecode::linear(*this, mps_resource_[x][y], Gecode::IRT_LQ, 1);
			}
		}

		// number of the machines on the field is equal to the defined types
		Gecode::count(*this, mps_type_, mps_count_, mps_types_);

		Gecode::branch(*this, mps_type_, Gecode::INT_VAR_RND(rg_), Gecode::INT_VAL_RND(rg_));
		Gecode::branch(*this, mps_angle_, Gecode::INT_VAR_RND(rg_), Gecode::INT_VAL_RND(rg_));

		options_.threads = 4;

		stop_ = new Gecode::Search::TimeStop(TIMEOUT_MS);

		options_.stop = stop_;

		search_ = new Gecode::DFS<MPSPlacing>(this, options_);

		solution = NULL;
	}

#if GECODE_VERSION_NUMBER >= 600200
	virtual MPSPlacing *
	copy()
	{
		return new MPSPlacing(*this);
	}
	explicit MPSPlacing(MPSPlacing &s) : Gecode::IntMinimizeSpace(s)
	{
		height_ = s.height_;
		width_  = s.width_;
		mps_type_.update(*this, s.mps_type_);
		mps_angle_.update(*this, s.mps_angle_);
		mps_count_.update(*this, s.mps_count_);
		mps_resource_.resize(width_ + 2);
		for (int x = 0; x < width_ + 2; x++) {
			mps_resource_[x].resize(height_ + 2);
			for (int y = 0; y < height_ + 2; y++) {
				mps_resource_[x][y].update(*this, s.mps_resource_[x][y]);
			}
		}
	};
#else
	MPSPlacing(bool share, MPSPlacing &s) : Gecode::IntMinimizeSpace(share, s)
	{
		height_ = s.height_;
		width_  = s.width_;
		mps_type_.update(*this, share, s.mps_type_);
		mps_angle_.update(*this, share, s.mps_angle_);
		mps_count_.update(*this, share, s.mps_count_);
		mps_resource_.resize(width_ + 2);
		for (int x = 0; x < width_ + 2; x++) {
			mps_resource_[x].resize(height_ + 2);
			for (int y = 0; y < height_ + 2; y++) {
				mps_resource_[x][y].update(*this, share, s.mps_resource_[x][y]);
			}
		}
	}

	MPSPlacing *
	copy(bool share)
	{
		return new MPSPlacing(share, *this);
	}
#endif

	Gecode::IntVar
	cost(void) const
	{
		return Gecode::IntVar(0);
	}

	bool
	solve()
	{
		bool connected_field = false;
		while (!connected_field) {
			if (solution) {
				delete solution;
			}

			stop_->reset();

			solution = search_->next();

			if (!solution) {
				return false;
			}
			connected_field = is_field_connected();
		}
		return true;
	}

	int
	index(int x, int y)
	{
		return (y * (width_ + 2) + x);
	}

	bool
	is_field_connected()
	{
		using namespace boost;
		if (solution) {
			// construct grid graph
			typedef boost::adjacency_list<vecS, vecS, undirectedS> Graph;

			Graph                         G((width_ + 2) * (height_ + 2));
			std::function<bool(int, int)> is_free_field = [this](int x, int y) {
				if (solution->mps_type_[index(x, y)].val() != EMPTY_ROT || x == 0 || x == width_ + 1
				    || y == 0 || y == height_ + 1) {
					return false;
				} else {
					// exclude the insertion zone
					if (y == 1 && x > width_ - 3) {
						return false;
					} else {
						return true;
					}
				}
			};
			int num_excluded_zones = 0;
			for (int x = 0; x < width_ + 2; x++) {
				for (int y = 0; y < height_ + 2; y++) {
					if (is_free_field(x, y)) {
						if (is_free_field(x + 1, y)) {
							add_edge(index(x, y), index(x + 1, y), G);
						}
						if (is_free_field(x, y + 1)) {
							add_edge(index(x, y), index(x, y + 1), G);
						}
					} else {
						num_excluded_zones++;
					}
				}
			}
			std::vector<int> component(num_vertices(G));
			int              num = boost::connected_components(G, &component[0]);
			return num == num_excluded_zones + 1;
		}
		return false;
	}

	bool
	get_solution(std::vector<MPSPlacingPlacing> &result)
	{
		if (!solution) {
			return false;
		}
		for (int x = 0; x < width_ + 2; x++) {
			for (int y = 0; y < height_ + 2; y++) {
				if (solution->mps_type_[index(x, y)].val() != EMPTY_ROT) {
					result.push_back(MPSPlacingPlacing(
					  x, y, solution->mps_type_[index(x, y)].val(), solution->mps_angle_[index(x, y)].val()));
				}
			}
		}

		return true;
	}

	Gecode::IntVarArray mps_type_;
	Gecode::IntVarArray mps_angle_;
	//Gecode::IntVarArray zone_blocked_;
	std::vector<std::vector<Gecode::IntVarArray>> mps_resource_;
	Gecode::IntArgs                               mps_types_;
	Gecode::IntVarArray                           mps_count_;
	int                                           height_;
	int                                           width_;
	std::set<int>                                 machines_;
	Gecode::DFS<MPSPlacing>                      *search_;
	MPSPlacing                                   *solution;
	Gecode::Rnd                                   rg_;
	Gecode::Search::Options                       options_;
	Gecode::Search::TimeStop                     *stop_;
};

#endif // MPS_PLACING_H
