#ifndef CszMICRO_HPP
#define CszMICRO_HPP
//NeedPiece Comp
//min heap
#define NPComp [](const std::pair<int32_t,std::shared_ptr<std::vector<int>>>& T_lhs,\
				  const std::pair<int32_t,std::shared_ptr<std::vector<int>>>& T_rhs)\
{\
	return (T_lhs.second)->size()> (T_rhs.second)->size();\
}

//DownSpeed Comp
//max heap
#define DSComp [](std::pair<int,uint32_t>& T_lhs,std::pair<int,uint32_t>& T_rhs)\
{\
	return T_lhs.second< T_rhs.second;\
}

//NeedPiece
//choke unchoke interested uninterested
#define CHOKE			0x01
#define UNCHOKE			0x02
#define INTERESTED		0x04
#define UNINTERESTED	0x08

//
//Slice Size
#define SLICESIZE       (16* 1024)

#endif //CszMICRO_HPP
