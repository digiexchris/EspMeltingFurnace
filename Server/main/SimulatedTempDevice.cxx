#include "TempDevice.hxx"

SimulatedTempDevice::SimulatedTempDevice()
{
	myResult.thermocouple_c = 25.0;
	myResult.thermocouple_f = 77.0;
	myResult.fault = TempFault::NONE;
}

void SimulatedTempDevice::SetTemp(float celsius)
{
	myResult.thermocouple_c = celsius;
	myResult.thermocouple_f = (celsius * 9.0 / 5.0) + 32.0;

	if (myResult.thermocouple_c > myHighFaultThreshold)
	{
		myResult.fault = TempFault::TCHIGH;
	}
	else if (myResult.thermocouple_c < myLowFaultThreshold)
	{
		myResult.fault = TempFault::TCLOW;
	}
	else
	{
		myResult.fault = TempFault::NONE;
	}
}

TempResult SimulatedTempDevice::GetResult()
{
	return myResult;
}

void SimulatedTempDevice::SetType(TempType type)
{
	myType = type;
}
void SimulatedTempDevice::SetTempFaultThresholds(float high, float low)
{
	myHighFaultThreshold = high;
	myLowFaultThreshold = low;

	if (myResult.thermocouple_c > myHighFaultThreshold)
	{
		myResult.fault = TempFault::TCHIGH;
	}
	else if (myResult.thermocouple_c < myLowFaultThreshold)
	{
		myResult.fault = TempFault::TCLOW;
	}
	else
	{
		myResult.fault = TempFault::NONE;
	}
}