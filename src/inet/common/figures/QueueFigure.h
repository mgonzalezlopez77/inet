//
// Copyright (C) OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_QUEUEFIGURE_H
#define __INET_QUEUEFIGURE_H

#include "inet/common/INETMath.h"

namespace inet {

class INET_API QueueFigure : public cRectangleFigure
{
  protected:
    double spacing = 2;
    double elementWidth = 16;
    double elementHeight = 4;
    std::vector<cRectangleFigure *> boxes;

  public:
    QueueFigure(const char *name = nullptr);

    void setValue(int value);
    void setMaxValue(int maxValue);
};

} // namespace inet

#endif // ifndef __INET_QUEUEFIGURE_H
