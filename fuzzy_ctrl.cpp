#include "fuzzy_ctrl.h"

FUZZY::FUZZY()
{
	
}

void FUZZY::value_hlt_e(float e)
{
    if (e < data_hlt_e[0])
		{
        muy_e[0] = 1;
        muy_e[1] = 0;
        muy_e[2] = 0;
        muy_e[3] = 0;
        muy_e[4] = 0;
		    muy_e[5] = 0;
		    muy_e[6] = 0;
		    muy_e[7] = 0;
		    muy_e[8] = 0;
    }
    else if (e < data_hlt_e[1])
		{
        muy_e[0] = (data_hlt_e[1] - e) / (data_hlt_e[1] - data_hlt_e[0]);
        muy_e[1] = (e - data_hlt_e[0]) / (data_hlt_e[1] - data_hlt_e[0]);
        muy_e[2] = 0;
        muy_e[3] = 0;
        muy_e[4] = 0;
		    muy_e[5] = 0;
		    muy_e[6] = 0;
		    muy_e[7] = 0;
		    muy_e[8] = 0;
    }
    else if(e < data_hlt_e[2])
		{
        muy_e[0] = 0;
        muy_e[1] = (data_hlt_e[2] - e) / (data_hlt_e[2] - data_hlt_e[1]);
        muy_e[2] = (e - data_hlt_e[1]) / (data_hlt_e[2] - data_hlt_e[1]);
        muy_e[3] = 0;
        muy_e[4] = 0;
		    muy_e[5] = 0;
		    muy_e[6] = 0;
		    muy_e[7] = 0;
		    muy_e[8] = 0;
    }
    else if(e < data_hlt_e[3])
		{
        muy_e[0] = 0;
        muy_e[1] = 0;
        muy_e[2] = (data_hlt_e[3] - e) / (data_hlt_e[3] - data_hlt_e[2]);
        muy_e[3] = (e - data_hlt_e[2]) / (data_hlt_e[3] - data_hlt_e[2]);
        muy_e[4] = 0;
		    muy_e[5] = 0;
		    muy_e[6] = 0;
		    muy_e[7] = 0;
		    muy_e[8] = 0;
    }
    else if(e < data_hlt_e[4])
		{
        muy_e[0] = 0;
        muy_e[1] = 0;
        muy_e[2] = 0;
        muy_e[3] = (data_hlt_e[4] - e) / (data_hlt_e[4] - data_hlt_e[3]);
        muy_e[4] = (e - data_hlt_e[3]) / (data_hlt_e[4] - data_hlt_e[3]);
		    muy_e[5] = 0;
		    muy_e[6] = 0;
		    muy_e[7] = 0;
		    muy_e[8] = 0;
    }
    else if(e < data_hlt_e[5])
		{
        muy_e[0] = 0;
        muy_e[1] = 0;
        muy_e[2] = 0;
        muy_e[3] = 0;
        muy_e[4] = (data_hlt_e[5] - e) / (data_hlt_e[5] - data_hlt_e[4]);
		    muy_e[5] = (e - data_hlt_e[4]) / (data_hlt_e[5] - data_hlt_e[4]);
		    muy_e[6] = 0;
		    muy_e[7] = 0;
		    muy_e[8] = 0;
    }
		else if(e < data_hlt_e[6])
		{
        muy_e[0] = 0;
        muy_e[1] = 0;
        muy_e[2] = 0;
        muy_e[3] = 0;
        muy_e[4] = 0;
		    muy_e[5] = (data_hlt_e[6] - e) / (data_hlt_e[6] - data_hlt_e[5]);
		    muy_e[6] = (e - data_hlt_e[5]) / (data_hlt_e[6] - data_hlt_e[5]);
		    muy_e[7] = 0;
		    muy_e[8] = 0;
    }
		else if(e < data_hlt_e[7])
		{
        muy_e[0] = 0;
        muy_e[1] = 0;
        muy_e[2] = 0;
        muy_e[3] = 0;
        muy_e[4] = 0;
		    muy_e[5] = 0;
		    muy_e[6] = (data_hlt_e[7] - e) / (data_hlt_e[7] - data_hlt_e[6]);
		    muy_e[7] = (e - data_hlt_e[6]) / (data_hlt_e[7] - data_hlt_e[6]);
		    muy_e[8] = 0;
    }
		else if(e < data_hlt_e[8])
		{
        muy_e[0] = 0;
        muy_e[1] = 0;
        muy_e[2] = 0;
        muy_e[3] = 0;
        muy_e[4] = 0;
		    muy_e[5] = 0;
		    muy_e[6] = 0;
		    muy_e[7] = (data_hlt_e[8] - e) / (data_hlt_e[8] - data_hlt_e[7]);
		    muy_e[8] = (e - data_hlt_e[7]) / (data_hlt_e[8] - data_hlt_e[7]);
    }
		else
		{
        muy_e[0] = 0;
        muy_e[1] = 0;
        muy_e[2] = 0;
        muy_e[3] = 0;
        muy_e[4] = 0;
		    muy_e[5] = 0;
		    muy_e[6] = 0;
		    muy_e[7] = 0;
		    muy_e[8] = 1;
    }
}

void FUZZY::value_hlt_e_dot(float e_dot)
{
    if (e_dot < data_hlt_e_dot[0])
		{
        muy_e_dot[0] = 1;
        muy_e_dot[1] = 0;
        muy_e_dot[2] = 0;
    }
    else if (e_dot < data_hlt_e_dot[1])
		{
        muy_e_dot[0] = (data_hlt_e_dot[1] - e_dot) / (data_hlt_e_dot[1] - data_hlt_e_dot[0]);
        muy_e_dot[1] = (e_dot - data_hlt_e_dot[0]) / (data_hlt_e_dot[1] - data_hlt_e_dot[0]);
        muy_e_dot[2] = 0;
    }
    else if(e_dot < data_hlt_e_dot[2])
		{
        muy_e_dot[0] = 0;
        muy_e_dot[1] = (data_hlt_e_dot[2] - e_dot) / (data_hlt_e_dot[2] - data_hlt_e_dot[1]);
        muy_e_dot[2] = (e_dot - data_hlt_e_dot[1]) / (data_hlt_e_dot[2] - data_hlt_e_dot[1]);
    }
    else
		{
        muy_e_dot[0] = 0;
        muy_e_dot[1] = 0;
        muy_e_dot[2] = 1;
    }
}

void FUZZY::find_beta()
{
	int a = 0;
	for(int i = 0;i < 3;i++)
	{
		for(int j = 0;j < 9;j++)
		{
		beta[a] = muy_e_dot[i]*muy_e[j];
		a++;
		}
   }
}

void FUZZY::rule_fuzzy()
{																		
    rule_kp[0] = data_kp[0];
    rule_kp[1] = data_kp[1];
    rule_kp[2] = data_kp[2];
    rule_kp[3] = data_kp[3];
    rule_kp[4] = data_kp[5];
    rule_kp[5] = data_kp[3];
    rule_kp[6] = data_kp[2];
    rule_kp[7] = data_kp[1];
    rule_kp[8] = data_kp[0];
	
    rule_kp[9] = data_kp[1];
    rule_kp[10] = data_kp[2];
    rule_kp[11] = data_kp[3];
    rule_kp[12] = data_kp[4];
    rule_kp[13] = data_kp[5];
    rule_kp[14] = data_kp[4];
    rule_kp[15] = data_kp[3];
    rule_kp[16] = data_kp[2];
    rule_kp[17] = data_kp[1];
	
    rule_kp[18] = data_kp[0];
    rule_kp[19] = data_kp[1];
    rule_kp[20] = data_kp[2];
    rule_kp[21] = data_kp[3];
    rule_kp[22] = data_kp[5];
    rule_kp[23] = data_kp[3];
    rule_kp[24] = data_kp[2];
	  rule_kp[25] = data_kp[1];
	  rule_kp[26] = data_kp[0];

    //////////////////////

    rule_ki[0] = data_ki[0];
    rule_ki[1] = data_ki[1];
    rule_ki[2] = data_ki[2];
    rule_ki[3] = data_ki[3];
    rule_ki[4] = data_ki[5];
    rule_ki[5] = data_ki[3];
    rule_ki[6] = data_ki[2];
    rule_ki[7] = data_ki[1];
    rule_ki[8] = data_ki[0];
		
    rule_ki[9] = data_ki[1];
    rule_ki[10] = data_ki[2];
    rule_ki[11] = data_ki[3];
    rule_ki[12] = data_ki[4];
    rule_ki[13] = data_ki[5];
    rule_ki[14] = data_ki[4];
    rule_ki[15] = data_ki[3];
    rule_ki[16] = data_ki[2];
    rule_ki[17] = data_ki[1];
		
    rule_ki[18] = data_ki[0];
    rule_ki[19] = data_ki[1];
    rule_ki[20] = data_ki[2];
    rule_ki[21] = data_ki[3];
    rule_ki[22] = data_ki[5];
    rule_ki[23] = data_ki[3];
    rule_ki[24] = data_ki[2];
	  rule_ki[25] = data_ki[1];
	  rule_ki[26] = data_ki[0];
		
		////////////////////////
		
	  rule_kd[0] = data_kd[6];
    rule_kd[1] = data_kd[5];
    rule_kd[2] = data_kd[4];
    rule_kd[3] = data_kd[3];
    rule_kd[4] = data_kd[1];
    rule_kd[5] = data_kd[3];
    rule_kd[6] = data_kd[4];
    rule_kd[7] = data_kd[5];
    rule_kd[8] = data_kd[6];
		
    rule_kd[9] = data_kd[5];
    rule_kd[10] = data_kd[4];
    rule_kd[11] = data_kd[3];
    rule_kd[12] = data_kd[2];
    rule_kd[13] = data_kd[1];
    rule_kd[14] = data_kd[2];
    rule_kd[15] = data_kd[3];
    rule_kd[16] = data_kd[4];
    rule_kd[17] = data_kd[5];
		
    rule_kd[18] = data_kd[6];
    rule_kd[19] = data_kd[5];
    rule_kd[20] = data_kd[4];
    rule_kd[21] = data_kd[3];
    rule_kd[22] = data_kd[1];
    rule_kd[23] = data_kd[3];
    rule_kd[24] = data_kd[4];
	  rule_kd[25] = data_kd[5];
	  rule_kd[26] = data_kd[6];
}

float FUZZY::defuzzy_kp()
{
    float temp1 = 0, temp2 = 0;
    for(int i=0;i<27;i++)
	{
		temp1 = temp1 + beta[i];
		temp2 = temp2 + beta[i]*rule_kp[i];
	}
    return (temp2/temp1);
}

float FUZZY::defuzzy_ki()
{
    float temp1=0, temp2=0;
    for(int i=0;i<27;i++)
		{
		temp1 = temp1 + beta[i];
   	temp2 = temp2 + beta[i]*rule_ki[i];   
		}
    return (temp2/temp1);
}

float FUZZY::defuzzy_kd()
{
    float temp1=0, temp2=0;
    for(int i=0;i<27;i++)
	{
		temp1 = temp1 + beta[i];
		temp2 = temp2 + beta[i]*rule_kd[i];   
	}
    return (temp2/temp1);
}