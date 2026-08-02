/**********************************************************************************************
 * Arduino PID Library - Version 1
 * by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 *
 * This Code is licensed under a Creative Commons Attribution-ShareAlike 3.0 Unported License.
 **********************************************************************************************/
#include "PID.h"

PID::PID()
{
	this->Kp = 3.0;
    this->Ki = 2.0;
    this->Kd = 1.0;
	this->Ts = 0.01;
    this->mode = "normal";
}

PID::PID(float Kp, float Ki, float Kd, float Ts)
{
	this->Kp = Kp;
    this->Ki = Ki;
    this->Kd = Kd;
	this->Ts = Ts;
    this->mode = "normal";
}

PID::PID(float Kp, float Ki, float Kd, float Ts, std::string mode)
{
	this->Kp = Kp;
    this->Ki = Ki;
    this->Kd = Kd;
	this->Ts = Ts;
    this->mode = mode;
}

void PID::set_Kp(float Kp)
{
	this->Kp = Kp;
}

void PID::set_Ki(float Ki)
{
	this->Ki = Ki;	
}

void PID::set_Kd(float Kd)
{
	this->Kd = Kd;	
}

void PID::set_mode(std::string mode)
{
	this->mode = mode;
}

float PID::compute(float ref, float y)
{
	ek = ref-y;
	uk = uk1 + Kp*(ek-ek1)+(Ki*Ts*(ek+ek1))/2+(Kd*(ek-2*ek1+ek2))/Ts;
	uk1 = uk;
	ek2 = ek1;
	ek1 = ek;

	return uk;
}

void PID::show_info()
{
	std::cout<<"this is information of PID controler"<<std::endl;
	std::cout<<"Kp: "<<this->Kp<<std::endl;
	std::cout<<"Ki: "<<this->Ki<<std::endl;
	std::cout<<"Kd: "<<this->Kd<<std::endl;
	std::cout<<"Ts: "<<this->Ts<<std::endl;
	std::cout<<"mode: "<<this->mode<<std::endl;
}

float PID::get_Kp()
{
	return this->Kp;
}

float PID::get_Ki()
{
	return this->Ki;
}

float PID::get_Kd()
{
	return this->Kd;
}

float PID::get_Ts()
{
	return this->Ts;
}

std::string PID::get_mode()
{
	return this->mode;
}