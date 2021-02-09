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

#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/search.hh>
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
public: // _challenge {1 = Expo || Nav, 2 = Produ, 3 = Half field}
	// _difficulty{1,2,3}
	MPSPlacing(int _width, int _height, int _challenge, int _difficulty)
	//MPSPlacing(int _width, int _height)
	{
		/* Stations for Challenges
                 * |Challenge   |Stations on field  |Allowed Stations   |
                 * |------------|-------------------|-------------------|
                 * |Explor      |2                  |1,2,3,4,5,6,7      |
                 * |            |3                  |                   |
                 * |            |4                  |                   |
                 * |------------|-------------------|-------------------|
                 * |Navigation  |2                  |1,2,3,4,5,6,7      |
                 * |            |3                  |                   |
                 * |            |4                  |                   |
                 * |------------|-------------------|-------------------|
                 * |Production  |2                  |1,2                |
                 * |+ Explo &   |3                  |1,2,4              |
                 * |  Production|4                  |1,2,4,5            |
                 * |------------|-------------------|-------------------|
                 * |Half Field  |7                  |1,2,3,4,5,6,7      |
                */
		int production_challenge[] = {0, 1, 2, 4, 5};
		int subtract_num_mps;

		//randomize station types for exploration and navigation
		std::vector<int> _mps{1, 2, 3, 4, 5, 6, 7};
		std::random_shuffle(_mps.begin(), _mps.end());

		height_ = _height;
		width_  = _width;

		// limit amount of mps stations on field
		switch (_difficulty) {
		case 1: subtract_num_mps = 5; break;
		case 2: subtract_num_mps = 4; break;
		case 3: subtract_num_mps = 3; break;
		default: subtract_num_mps = 0; break;
		}
		mps_type_  = Gecode::IntVarArray(*this,
                                    (height_ + 2) * (width_ + 2),
                                    EMPTY_ROT,
                                    NUM_MPS - subtract_num_mps);
		mps_angle_ = Gecode::IntVarArray(*this, (height_ + 2) * (width_ + 2), EMPTY_ROT, ANGLE_315);
		//zone_blocked_ = Gecode::IntVarArray(*this, (height_+2) * (width_+2) , 0, 1);

		rg_ = Gecode::Rnd(time(NULL));

		std::vector<int> types;
		for (int i = 0; i <= NUM_MPS; i++) {
			types.push_back(i);
		}
		std::vector<int> allowed_types;
		// fill types vector with allowed mps station types for each challenge according to table above
		switch (_challenge) {
		case 1:
			// push back 0, because or random mps stations in _mps vector
			allowed_types.push_back(0);
			for (int i = 1; i <= NUM_MPS - subtract_num_mps; i++) {
				allowed_types.push_back(_mps.back());
				_mps.pop_back();
			}
			std::sort(allowed_types.begin(), allowed_types.end());
			break;
		case 2:
			for (int i = 0; i <= NUM_MPS - subtract_num_mps; i++) {
				allowed_types.push_back(production_challenge[i]);
			}
			break;
		case 3:
			for (int i = 0; i <= NUM_MPS; i++) {
				allowed_types.push_back(i);
			}
			break;
		}

		mps_types_ = Gecode::IntArgs(types);
		mps_count_ = Gecode::IntVarArray(*this, NUM_MPS + 1, EMPTY_ROT, (height_ + 2) * (width_ + 2));

		mps_resource_.resize(width_ + 2);
		for (int x = 0; x < width_ + 2; x++) {
			mps_resource_[x].resize(height_ + 2);
			for (int y = 0; y < height_ + 2; y++) {
				mps_resource_[x][y] = Gecode::IntVarArray(*this, NUM_MPS, 0, 1);
			}
		}

		// counting constraint for number of different machines
		for (int i = 1; i <= NUM_MPS; i++) {
			if (std::binary_search(allowed_types.begin(), allowed_types.end(), i)) {
				Gecode::rel(*this, mps_count_[i], Gecode::IRT_EQ, 1);
			} else {
				Gecode::rel(*this, mps_count_[i], Gecode::IRT_EQ, 0);
			}
		}

		/*Gecode::rel(*this, mps_count_[BASE], Gecode::IRT_EQ, 1);
		Gecode::rel(*this, mps_count_[CAP1], Gecode::IRT_EQ, 1);
		Gecode::rel(*this, mps_count_[CAP2], Gecode::IRT_EQ, 1);
		Gecode::rel(*this, mps_count_[RING1], Gecode::IRT_EQ, 1);
		Gecode::rel(*this, mps_count_[RING2], Gecode::IRT_EQ, 1);
		Gecode::rel(*this, mps_count_[STORAGE], Gecode::IRT_EQ, 1);
                Gecode::rel(*this, mps_count_[DELIVERY], Gecode::IRT_EQ, 1);*/

		// an EMPTY_ROT zone has no orientation
		for (int i = 0; i < (height_ + 2) * (width_ + 2); i++) {
			Gecode::rel(*this, (mps_type_[i] == EMPTY_ROT) >> (mps_angle_[i] == EMPTY_ROT));
		}

		// placed machine must have an orientation and blocks a zone
		for (int i = 0; i < (height_ + 2) * (width_ + 2); i++) {
			Gecode::rel(*this, (mps_type_[i] != EMPTY_ROT) >> (mps_angle_[i] != EMPTY_ROT));
			//Gecode::rel(*this, (zone_blocked_[i]==1) >> (mps_type_[i]==EMPTY_ROT));
		}

		// mark the border blocked
		for (int x = 0; x < width_ + 2; x++) {
			Gecode::rel(*this, mps_type_[index(x, 0)] == EMPTY_ROT);
			Gecode::rel(*this, mps_resource_[x][0][BASE - 1] == 1);
			for (int t = 1; t < NUM_MPS; t++) {
				Gecode::rel(*this, mps_resource_[x][0][BASE - 1] == 1);
			}
		}
		for (int x = 0; x < width_ + 2; x++) {
			Gecode::rel(*this, mps_type_[index(x, height_ + 1)] == EMPTY_ROT);
			Gecode::rel(*this, mps_resource_[x][height_ + 1][BASE - 1] == 1);
			for (int t = 1; t < NUM_MPS; t++) {
				Gecode::rel(*this, mps_resource_[x][height_ + 1][BASE - 1] == 1);
			}
		}
		for (int y = 0; y < height_ + 2; y++) {
			Gecode::rel(*this, mps_type_[index(0, y)] == EMPTY_ROT);
			Gecode::rel(*this, mps_resource_[0][y][BASE - 1] == 1);
			for (int t = 1; t < NUM_MPS; t++) {
				Gecode::rel(*this, mps_resource_[0][y][BASE - 1] == 1);
			}
		}
		for (int y = 0; y < height_ + 2; y++) {
			Gecode::rel(*this, mps_type_[index(width_ + 1, y)] == EMPTY_ROT);
			Gecode::rel(*this, mps_resource_[width_ + 1][y][BASE - 1] == 1);
			for (int t = 1; t < NUM_MPS; t++) {
				Gecode::rel(*this, mps_resource_[width_ + 1][y][BASE - 1] == 1);
			}
		}

		// manage border cases

		// along x border
		for (int x = 1; x <= width_; x++) {
			// BASE
			for (int t = BASE; t <= BASE; t++) {
				Gecode::rel(*this,
				            (mps_type_[index(x, 1)] == t) >> ((mps_angle_[index(x, 1)] == ANGLE_0)
				                                              || (mps_angle_[index(x, 1)] == ANGLE_180)));
				Gecode::rel(*this,
				            (mps_type_[index(x, height_)] == t)
				              >> ((mps_angle_[index(x, height_)] == ANGLE_0)
				                  || (mps_angle_[index(x, height_)] == ANGLE_180)));
			}
		}
		for (int x = 1; x <= width_; x++) {
			// CAP1, CAP2, RING1, RING 2
			for (int t = CAP1; t <= RING2; t++) {
				Gecode::rel(*this,
				            (mps_type_[index(x, 1)] == t)
				              >> ((mps_angle_[index(x, 1)]
				                   == ANGLE_0))); //|| (mps_angle_[index(x,1)] == ANGLE_180)));
				Gecode::rel(*this,
				            (mps_type_[index(x, height_)] == t)
				              >> ((mps_angle_[index(x, height_)]
				                   == ANGLE_180))); //|| (mps_angle_[index(x, height_)] == ANGLE_180)));
			}
		}
		for (int x = 1; x <= width_; x++) {
			// STORAGE, DELIVERY
			for (int t = STORAGE; t <= DELIVERY; t++) {
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
			// BASE, CAP1, CAP2, RING1, RING 2
			for (int t = BASE; t <= RING2; t++) {
				Gecode::rel(*this,
				            (mps_type_[index(x, 2)] == t) >> ((mps_angle_[index(x, 2)] == ANGLE_0)
				                                              || (mps_angle_[index(x, 2)] == ANGLE_180)));
			}
		}
		for (int x = width_ - 1; x <= width_; x++) {
			// STORAGE, DELIVERY
			for (int t = STORAGE; t <= DELIVERY; t++) {
				Gecode::rel(*this,
				            (mps_type_[index(x, 2)] == t) >> ((mps_angle_[index(x, 1)] != ANGLE_225)
				                                              && (mps_angle_[index(x, 1)] != ANGLE_270)
				                                              && (mps_angle_[index(x, 1)] != ANGLE_315)));
			}
		}

		// along y border
		for (int y = 1; y <= height_; y++) {
			// BASE
			for (int t = BASE; t <= BASE; t++) {
				Gecode::rel(*this,
				            (mps_type_[index(1, y)] == t) >> ((mps_angle_[index(1, y)] == ANGLE_90)
				                                              || (mps_angle_[index(1, y)] == ANGLE_270)));
				Gecode::rel(*this,
				            (mps_type_[index(width_, y)] == t)
				              >> ((mps_angle_[index(width_, y)] == ANGLE_90)
				                  || (mps_angle_[index(width_, y)] == ANGLE_270)));
			}
		}
		for (int y = 1; y <= height_; y++) {
			// CAP1, CAP2, RING1, RING 2
			for (int t = CAP1; t <= RING2; t++) {
				Gecode::rel(*this,
				            (mps_type_[index(1, y)] == t) >> ((mps_angle_[index(1, y)] == ANGLE_90)
				                                              || (mps_angle_[index(1, y)] == ANGLE_270)));
				Gecode::rel(*this,
				            (mps_type_[index(width_, y)] == t)
				              >> ((mps_angle_[index(width_, y)]
				                   == ANGLE_270))); // || (mps_angle_[index(width_, y)] == ANGLE_270)));
			}
		}
		for (int y = 1; y <= height_; y++) {
			// STORAGE, DELIVERY
			for (int t = STORAGE; t <= DELIVERY; t++) {
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

		// BASE, CAP1, CAP2, RING1, RING 2
		for (int t = BASE; t <= RING2; t++) {
			Gecode::rel(*this,
			            (mps_type_[index(width_ - 3, 1)] == t)
			              >> ((mps_angle_[index(width_ - 3, 1)] == ANGLE_90)
			                  || (mps_angle_[index(width_ - 3, 1)] == ANGLE_270)));
		}

		// STORAGE, DELIVERY
		for (int t = STORAGE; t <= DELIVERY; t++) {
			Gecode::rel(*this,
			            (mps_type_[index(width_ - 3, 1)] == t)
			              >> ((mps_angle_[index(width_ - 3, 1)] != ANGLE_135)
			                  && (mps_angle_[index(width_ - 3, 1)] != ANGLE_180)
			                  && (mps_angle_[index(width_ - 3, 1)] != ANGLE_225)));
		}

		//avoid problems on entry wall
		Gecode::rel(*this, mps_resource_[width_][1][BASE - 1] == 1);
		Gecode::rel(*this, mps_resource_[width_ - 1][1][BASE - 1] == 1);
		Gecode::rel(*this, mps_resource_[width_ - 2][1][BASE - 1] == 1);

		// insert blocking contraints

		//block entry zones
		Gecode::rel(*this, mps_type_[index(width_, 1)] == EMPTY_ROT);
		Gecode::rel(*this, mps_type_[index(width_ - 1, 1)] == EMPTY_ROT);
		Gecode::rel(*this, mps_type_[index(width_ - 2, 1)] == EMPTY_ROT);
		//Gecode::rel(*this, mps_type_[index(width_ - 2, 2)] == EMPTY_ROT);

		// insert blocking contraints
		for (int x = 1; x <= width_; x++) {
			for (int y = 1; y <= height_; y++) {
				// Angle 0

				// BASE, CAP1, CAP2, RING1, RING2, STORAGE
				for (int t = BASE; t <= STORAGE; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_0) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_0) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_0) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y][t - 1] == 1));
				}
				// DELIVERY
				for (int t = DELIVERY; t <= DELIVERY; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_0) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_0) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y][t - 1] == 1));
				}

				// Angle 45

				// BASE, CAP1, CAP2, RING1, RING, STORAGE
				for (int t = BASE; t <= STORAGE; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_45) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_45) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_45) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y][t - 1] == 1));

					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_45) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y - 1][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_45) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y + 1][t - 1] == 1));

					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_45) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y + 1][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_45) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y - 1][t - 1] == 1));
				}
				// DELIVERY
				for (int t = DELIVERY; t <= DELIVERY; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_45) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_45) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_45) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y + 1][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_45) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y + 1][t - 1] == 1));
				}

				// Angle 90

				// BASE, CAP1, CAP2, RING1, RING2, STORAGE
				for (int t = BASE; t <= STORAGE; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_90) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_90) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y - 1][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_90) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y + 1][t - 1] == 1));
				}
				// DELIVERY
				for (int t = DELIVERY; t <= DELIVERY; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_90) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_90) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y + 1][t - 1] == 1));
				}

				// Angle 135

				// BASE, CAP1, CAP2, RING1, RING2, STORAGE
				for (int t = BASE; t <= STORAGE; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_135) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));

					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_135) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_135) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y][t - 1] == 1));

					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_135) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y - 1][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_135) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y + 1][t - 1] == 1));

					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_135) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y + 1][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_135) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y - 1][t - 1] == 1));
				}
				// DELIVERY
				for (int t = DELIVERY; t <= DELIVERY; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_135) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_135) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_135) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y + 1][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_135) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y + 1][t - 1] == 1));
				}

				// Angle 180

				// BASE, CAP1, CAP2, RING1, RING2, STORAGE
				for (int t = BASE; t <= STORAGE; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_180) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_180) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_180) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y][t - 1] == 1));
				}
				// DELIVERY
				for (int t = DELIVERY; t <= DELIVERY; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_180) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_180) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y][t - 1] == 1));
				}

				// Angle 225

				// BASE, CAP1, CAP2, RING1, RING2, STORAGE
				for (int t = BASE; t <= STORAGE; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_225) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_225) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_225) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y][t - 1] == 1));

					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_225) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y - 1][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_225) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y + 1][t - 1] == 1));

					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_225) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y - 1][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_225) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y + 1][t - 1] == 1));
				}
				// DELIVERY
				for (int t = DELIVERY; t <= DELIVERY; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_225) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_225) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_225) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y - 1][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_225) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y - 1][t - 1] == 1));
				}

				// Angle 270

				// BASE, CAP1, CAP2, RING1, RING2, STORAGE
				for (int t = BASE; t <= STORAGE; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_270) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_270) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y - 1][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_270) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y + 1][t - 1] == 1));
				}
				// DELIVERY
				for (int t = DELIVERY; t <= DELIVERY; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_270) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_270) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y - 1][t - 1] == 1));
				}

				// Angle 315

				// BASE, CAP1, CAP2, RING1, RING2, STORAGE
				for (int t = BASE; t <= STORAGE; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_315) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_315) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_315) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y][t - 1] == 1));

					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_315) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y - 1][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_315) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y + 1][t - 1] == 1));

					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_315) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x + 1][y + 1][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_315) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y - 1][t - 1] == 1));
				}
				// DELIVERY
				for (int t = DELIVERY; t <= DELIVERY; t++) {
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_315) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_315) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_315) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x][y - 1][t - 1] == 1));
					Gecode::rel(*this,
					            ((mps_angle_[index(x, y)] == ANGLE_315) && (mps_type_[index(x, y)] == t))
					              >> (mps_resource_[x - 1][y - 1][t - 1] == 1));
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

		//horiziontal
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

		// restrict zone useage to max 1
		for (int x = 0; x < width_ + 2; x++) {
			for (int y = 0; y < height_ + 2; y++) {
				Gecode::linear(*this, mps_resource_[x][y], Gecode::IRT_LQ, 1);
			}
		}

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
		if (solution) {
			delete solution;
		}

		stop_->reset();

		solution = search_->next();

		if (!solution) {
			return false;
		}

		return true;
	}

	int
	index(int x, int y)
	{
		return (y * (width_ + 2) + x);
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
	Gecode::DFS<MPSPlacing> *                     search_;
	MPSPlacing *                                  solution;
	Gecode::Rnd                                   rg_;
	Gecode::Search::Options                       options_;
	Gecode::Search::TimeStop *                    stop_;
};

#endif // MPS_PLACING_H
