#pragma once

#include "Region.h"

class DialGuage : public ClusterElement
{
public:
	ClusterElement();
	virtual ~ClusterElement();
	virtual Region Update();
	virtual Region Draw();
protected:
};


