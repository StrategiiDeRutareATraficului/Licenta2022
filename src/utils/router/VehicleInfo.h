#pragma once
class VehicleInfo
{
private:
	double segmentMaxSpeed;
public:
	VehicleInfo(double segmentMaxSpeed)
	{
		this->segmentMaxSpeed = segmentMaxSpeed;
	}

	double getSegmentMaxSpeedMaxSpeed() {
		return this->segmentMaxSpeed;
	}

	void setSegmentMaxSpeedMaxSpeed(double segmentMaxSpeed) {
		this->segmentMaxSpeed = segmentMaxSpeed;
	}
};

