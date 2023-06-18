void matrix3d_Multiply( double A[4][4], double B[4][4] );
void matrix3d_Identity( double M[4][4] );
void matrix3d_Translate( double M[4][4], double x, double y, double z );
void matrix3d_RotX( double M[4][4], double thx_sin, double thx_cos );
void matrix3d_RotY( double M[4][4], double thy_sin, double thy_cos );
void matrix3d_RotZ( double M[4][4], double thz_sin, double thz_cos );
