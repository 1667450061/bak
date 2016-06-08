/*
*curve for coolpad battery P2.1500mAh,4.8wh.type:CPLD-47.
*/

#ifdef BATT_VOLT_SAMPLE_NUM
#undef BATT_VOLT_SAMPLE_NUM
#endif

#define BATT_VOLT_SAMPLE_NUM 26

static cap_curve discharge_curve[] = {
	{
		.cur = 200,/* 200ma */
			.table = {
				[0] ={99,4141},
				[1] ={96,4108},
				[2] ={92,4069},
				[3] ={88,4036},
				[4] ={84,3999},
				[5] ={80,3967},
				[6] ={76,3937},
				[7] ={72,3910},
				[8] ={68,3882},
				[9] ={64,3856},
				[10]={60,3831},
				[11]={56,3806},
				[12]={52,3786},
				[13]={48,3770},
				[14]={44,3756},
				[15]={40,3744},
				[16]={36,3738},
				[17]={32,3732},
				[18]={28,3728},
				[19]={24,3723},
				[20]={20,3718},
				[21]={16,3710},
				[22]={12,3701},
				[23]={8,3681},
				[24]={4,3641},
				[25]={0,3597},
		},
	},
	{
		.cur = 201,/* 500ma */
			.table = {
				[0] ={99,4104},
				[1] ={96,4056},
				[2] ={92,4020},
				[3] ={88,3983},
				[4] ={84,3950},
				[5] ={80,3918},
				[6] ={76,3887},
				[7] ={72,3858},
				[8] ={68,3834},
				[9] ={64,3808},
				[10]={60,3786},
				[11]={56,3765},
				[12]={52,3748},
				[13]={48,3729},
				[14]={44,3712},
				[15]={40,3702},
				[16]={36,3691},
				[17]={32,3684},
				[18]={28,3677},
				[19]={24,3669},
				[20]={20,3663},
				[21]={16,3654},
				[22]={12,3647},
				[23]={8,3637},
				[24]={4,3622},
				[25]={0,3601},
		},
				},
};

static cap_curve charge_curve[] = {
	{//switch on
		.cur = 3,
			.table = {
				[0]= {99,4233},
				[1]= {96,4228},
				[2]= {92,4208},
				[3]= {88,4177},
				[4]= {84,4145},
				[5]= {80,4108},
				[6]= {76,4060},
				[7]= {72,4033},
				[8]= {68,4005},
				[9]= {64,3979},
				[10]={60,3956},
				[11]={56,3933},
				[12]={52,3918},
				[13]={48,3898},
				[14]={44,3885},
				[15]={40,3873},
				[16]={36,3833},
				[17]={32,3825},
				[18]={28,3820},
				[19]={24,3810},
				[20]={20,3797},
				[21]={16,3778},
				[22]={12,3768},
				[23]={8,3748},
				[24]={4,3724},
				[25]={0,3601},
		},
	},
	{//switch off
		.cur = 399,
			.table = {
#if 0
				[0]= {99, 4199},
				[1]= {96, 4185},
				[2]= {92, 4177},
				[3]= {88, 4139},
				[4]= {84, 4099},
				[5]= {80, 4058},
				[6]= {76, 4028},
				[7]= {72, 4004},
				[8]= {68, 3980},
				[9]= {64, 3957},
				[10]={60, 3938},
				[11]={56, 3919},
				[12]={52, 3900},
				[13]={48, 3884},
				[14]={44, 3868},
				[15]={40, 3857},
				[16]={36, 3847},
				[17]={32, 3841},
				[18]={28, 3836},
				[19]={24, 3828},
				[20]={20, 3822},
				[21]={16, 3802},
				[22]={12, 3779},
				[23]={8,  3753},
				[24]={4,  3732},
				[25]={0,  3710},
#else
				{99, 4200},
				{96, 4174},
				{92, 4137},
				{88, 4104},
				{84, 4068},
				{80, 4038},
				{76, 4010},
				{72, 3984},
				{68, 3960},
				{64, 3938},
				{60, 3920},
				{56, 3901},
				{52, 3885},
				{48, 3865},
				{44, 3851},
				{40, 3841},
				{36, 3834},
				{32, 3828},
				{28, 3825},
				{24, 3821},
				{20, 3816},
				{16, 3808},
				{12, 3789},
				{8, 3752},
				{4, 3703},
				{0, 3652},
#endif
		},
				},
};