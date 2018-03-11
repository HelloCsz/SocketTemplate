#ifndef CszMICRO_HPP
#define CszMICRO_HPP
//NeedPiece Comp
//vector min heap or list sort
#define NPComp [](const std::shared_ptr<DataType>& T_lhs\
				  ,const std::shared_ptr<DataType>& T_rhs)\
{\
	return (T_lhs->queue).size()< (T_rhs->queue).size();\
}

//DownSpeed Comp
//max heap or list sort
#define DSComp [](const DataType& T_lhs,const DataType& T_rhs)\
{\
    if (T_lhs.status.peer_interested> T_rhs.status.peer_interested)\
    {\
        return true;\
    }\
    else if (T_lhs.status.peer_interested== T_rhs.status.peer_interested)\
    {\
        return T_lhs.total> T_rhs.total;\
    }\
	else\
	{\
		return false;\
	}\
}


//
//Slice Size
#define SLICESIZE       (16* 1024)

//socket timeout s to us
#define TIMEOUT300MS	{300000}
#define TIMEOUT1000MS   (1000000)
#define TIMEOUT2000MS   (2000000)
#define TIMEOUT3000MS   (3000000)

//in select switch operator()
#define THREADNUM		(150)

#endif //CszMICRO_HPP
