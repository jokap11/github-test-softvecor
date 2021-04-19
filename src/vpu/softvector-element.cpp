/*
 * Copyright [2020] [Technical University of Munich]
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
//////////////////////////////////////////////////////////////////////////////////////
/// \file softvector-element.cpp
/// \brief Extended element type for softvector representation
/// \date 07/03/2020
//////////////////////////////////////////////////////////////////////////////////////

#include "vpu/softvector-types.hpp"

inline size_t get_shiftamount(size_t target_width_bits, const uint8_t* rhs){
	size_t numberofbits = 0;
	size_t shiftamount = 0;
	while(target_width_bits){
		target_width_bits = target_width_bits >> 1;
		shiftamount |= rhs[numberofbits/8] & (1 << numberofbits);
		numberofbits++;
	}
	return(shiftamount);
}

inline size_t get_shiftamount(size_t target_width_bits, uint64_t rhs){
	size_t numberofbits = 0;
	size_t shiftamount = 0;
	while(target_width_bits){
		target_width_bits = target_width_bits >> 1;
		shiftamount |= rhs & (1 << numberofbits);
		numberofbits++;
	}
	return(shiftamount);
}

inline uint8_t arr_shift_left(uint8_t* dat, size_t size){
    uint8_t cin = 0;
	for(size_t i = 0; i < size; ++i){
    	uint8_t cout = (dat[i] & 0x80) ? 0x1:0;
    	dat[i] = cin | (dat[i] << 1);
    	cin = cout;
    }
	return(cin);
}

inline uint8_t arr_shift_right(uint8_t* dat, size_t size, bool arith = false){
	uint8_t msb = dat[size-1] & 0x80;
	uint8_t cin = arith ? msb : 0;
	for(int i = size -1; i >= 0 ; --i){
		uint8_t cout = (dat[i] & 0x01) ? 0x80 : 0;
		dat[i] = cin | (dat[i] >> 1);
		cin = cout;
	}
	return(cin);
}

SVElement& SVElement::operator=(const SVElement& rhs){
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		(*this)[i_byte] = rhs[i_byte];
	}
	return (*this);
}

SVElement& SVElement::operator=(const int64_t rhs){
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		if(i_byte < 8) (*this)[i_byte] = 0xFF&(rhs >> 8*i_byte);
		else (*this)[i_byte] = (rhs >= 0) ? 0 : 0xFF;
	}
	return (*this);
}

SVElement& SVElement::operator++(){
	*this = *this + 1;
	return(*this);
}

SVElement SVElement::operator++(int){
	SVElement temp(*this);
	*this = *this + 1;
	return(temp);
}

SVElement& SVElement::operator--(){
	*this = *this + (-1);
	return(*this);
}

SVElement SVElement::operator--(int){
	SVElement temp(*this);
	*this = *this + (-1);
	return(temp);
}

void SVElement::twos_complement(void){
	for(size_t i = 0; i < width_in_bits_/8; ++i){
		mem_[i] = ~(mem_[i]);
	}
	++(*this);
}

SVElement SVElement::operator+(const SVElement& rhs) const {
	SVElement ret(width_in_bits_);
	uint8_t carry = 0;
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint16_t x = (*this)[i_byte] + rhs[i_byte] + carry;
		carry = (x & (0xFF00)) ? 1 : 0;
		ret[i_byte] = static_cast<uint8_t>(x);
	}
	return(ret);
}

SVElement SVElement::operator+(const int64_t rhs) const {
	SVElement ret(width_in_bits_);
	uint8_t carry = 0;
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint16_t x = 0;
		if(i_byte < 8)  x = (*this)[i_byte] + (0xFF&(rhs >> 8*i_byte)) + carry;
		else x = (*this)[i_byte] + ((rhs >= 0) ? 0 : 0xFF) + carry;
		carry = (x&0xFF00)? 1 : 0;
		ret[i_byte] = static_cast<uint8_t>(x);
	}
	return (ret);
}

SVElement SVElement::operator-(const SVElement& rhs) const {
	SVElement ret(width_in_bits_);
	SVElement twos(rhs);
	twos.twos_complement();
	uint8_t carry = 0;
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint16_t x = (*this)[i_byte] + twos[i_byte] + carry;
		carry = (x & (0xFF00)) ? 1 : 0;
		ret[i_byte] = static_cast<uint8_t>(x);
	}
	return(ret);
}

SVElement SVElement::operator-(const int64_t rhs) const {
	SVElement ret(width_in_bits_);
	SVElement twos(width_in_bits_);
	twos = rhs;
	return(*this - twos);
}

SVElement SVElement::operator&(const SVElement& rhs) const {
	SVElement ret(width_in_bits_);
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint16_t x = (*this)[i_byte] & rhs[i_byte];
		ret[i_byte] = static_cast<uint8_t>(x);
	}
	return(ret);
}

SVElement SVElement::operator&(const int64_t rhs) const {
	SVElement ret(width_in_bits_);
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint16_t x = 0;
		if(i_byte < 8)  x = (*this)[i_byte] & (0xFF&(rhs >> 8*i_byte));
		else x = (*this)[i_byte] & ((rhs >= 0) ? 0 : 0xFF);
		ret[i_byte] = static_cast<uint8_t>(x);
	}
	return (ret);
}

SVElement SVElement::operator|(const SVElement& rhs) const {
	SVElement ret(width_in_bits_);
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint16_t x = (*this)[i_byte] | rhs[i_byte];
		ret[i_byte] = static_cast<uint8_t>(x);
	}
	return(ret);
}

SVElement SVElement::operator|(const int64_t rhs) const {
	SVElement ret(width_in_bits_);
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint16_t x = 0;
		if(i_byte < 8)  x = (*this)[i_byte] | (0xFF&(rhs >> 8*i_byte));
		else x = (*this)[i_byte] | ((rhs >= 0) ? 0 : 0xFF);
		ret[i_byte] = static_cast<uint8_t>(x);
	}
	return (ret);
}

SVElement SVElement::operator^(const SVElement& rhs) const {
	SVElement ret(width_in_bits_);
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint16_t x = (*this)[i_byte] ^ rhs[i_byte];
		ret[i_byte] = static_cast<uint8_t>(x);
	}
	return(ret);
}

SVElement SVElement::operator^(const int64_t rhs) const {
	SVElement ret(width_in_bits_);
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint16_t x = 0;
		if(i_byte < 8)  x = (*this)[i_byte] ^ (0xFF&(rhs >> 8*i_byte));
		else x = (*this)[i_byte] ^ ((rhs >= 0) ? 0 : 0xFF);
		ret[i_byte] = static_cast<uint8_t>(x);
	}
	return (ret);
}

SVElement SVElement::operator<<(const SVElement& rhs) const {
	SVElement ret(*this);

	size_t shiftamount = get_shiftamount(width_in_bits_, &(rhs[0]));
	while (shiftamount--) {
		arr_shift_left(&(ret[0]), width_in_bits_/8);
	}
	return(ret);
}

SVElement SVElement::operator<<(const uint64_t rhs) const {
	SVElement ret(*this);

	size_t shiftamount = get_shiftamount(width_in_bits_, rhs);
    while (shiftamount--) {
    	arr_shift_left(&(ret[0]), width_in_bits_/8);
    }
    return(ret);
}

SVElement SVElement::operator>>(const SVElement& rhs) const {
	SVElement ret(*this);

	size_t shiftamount = get_shiftamount(width_in_bits_, &(rhs[0]));
    while (shiftamount--) {
    	arr_shift_right(&(ret[0]), width_in_bits_/8, true);
    }
    return(ret);
}

SVElement SVElement::operator>>(const uint64_t rhs) const {
	SVElement ret(*this);

	size_t shiftamount = get_shiftamount(width_in_bits_, rhs);
    while (shiftamount--) {
    	arr_shift_right(&(ret[0]), width_in_bits_/8, true);
    }
    return(ret);
}

SVElement SVElement::srl(const SVElement& rhs) const {
	SVElement ret(*this);

	size_t shiftamount = get_shiftamount(width_in_bits_, &(rhs[0]));
    while (shiftamount--) {
    	arr_shift_right(&(ret[0]), width_in_bits_/8, false);
    }
    return(ret);
}

SVElement SVElement::srl(const uint64_t rhs) const {
	SVElement ret(*this);

	size_t shiftamount = get_shiftamount(width_in_bits_, rhs);
    while (shiftamount--) {
    	arr_shift_right(&(ret[0]), width_in_bits_/8, false);
    }
    return(ret);
}

bool SVElement::operator==(const SVElement& rhs) const{
	bool ret = true;
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		if((*this)[i_byte] != rhs[i_byte] ) {
			ret = false;
			break;
		}
	}
	return (ret);
}

bool SVElement::operator==(const int64_t rhs) const{
	bool ret = true;
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint8_t x = (i_byte < 8) ? (rhs >> i_byte*8) : ((rhs >= 0) ? 0x00 : 0xFF);
		if(	(*this)[i_byte] != x ){
			ret = false;
			break;
		}
	}
	return (ret);
}

bool SVElement::operator!=(const SVElement& rhs) const{
	return( !((*this)==rhs) );
}

bool SVElement::operator!=(const int64_t rhs) const{
	return( !((*this)==rhs) );
}

// signed comparisons
bool SVElement::operator<(const SVElement& rhs) const{
	SVElement x(*this);
	x = *this - rhs;
	if(x[x.width_in_bits_/8 -1] & 0x80) return(true);
	else return(false);
}

bool SVElement::operator<(const int64_t rhs) const{
	SVElement x(*this);
	x = *this - rhs;
	if(x[x.width_in_bits_/8 -1] & 0x80) return(true);
	else return(false);
}

bool SVElement::operator<=(const SVElement& rhs) const{
	return (!(*this > rhs));
}
bool SVElement::operator<=(const int64_t rhs) const{
	return (!(*this > rhs));
}

bool SVElement::operator>(const SVElement& rhs) const{
	return(rhs < *this);
}

bool SVElement::operator>(const int64_t rhs) const{
	SVElement x(*this);
	x = *this - rhs;
	if(x[x.width_in_bits_/8 -1] & 0x80) return(false);
	else return(true);
}

bool SVElement::operator>=(const SVElement& rhs) const{
	return (!(*this < rhs));
}

bool SVElement::operator>=(const int64_t rhs) const{
	return (!(*this < rhs));
}

// unsigned comparisons
bool SVElement::op_u_lt(const SVElement& rhs) const{
	bool ret = true;
	for (int i_byte = width_in_bits_/8 -1; i_byte > 0 ; --i_byte){
		if(	(*this)[i_byte] > rhs[i_byte] ){
			ret = false;
			break;
		}
	}
	if((*this)[0] >= rhs[0]) ret = false;
	return (ret);
}

bool SVElement::op_u_lt(const uint64_t rhs) const{
	bool ret = true;
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint8_t x = (i_byte < 8) ? (rhs >> i_byte*8) : 0;
		if(	(*this)[i_byte] > x ){
			ret = false;
			break;
		}
	}
	if((*this)[0] >= (rhs&0xff)) ret = false;
	return (ret);
}

bool SVElement::op_u_lte(const SVElement& rhs) const{
	return(!(this->op_u_gt(rhs)));
}
bool SVElement::op_u_lte(const uint64_t rhs) const{
	return(!(this->op_u_gt(rhs)));
}

bool SVElement::op_u_gt(const SVElement& rhs) const{
	return (rhs.op_u_lt(*this));
}

bool SVElement::op_u_gt(const uint64_t rhs) const{
	bool ret = true;
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint8_t x = (i_byte < 8) ? (rhs >> i_byte*8) : 0;
		if(	(*this)[i_byte] < x ){
			ret = false;
			break;
		}
	}
	if((*this)[0] <= (rhs&0xff)) ret = false;
	return (ret);
}

bool SVElement::op_u_gte(const SVElement& rhs) const{
	return(!(this->op_u_lt(rhs)));
}
bool SVElement::op_u_gte(const uint64_t rhs) const{
	return(!(this->op_u_lt(rhs)));
}




SVElement& SVElement::s_add(const SVElement& opL, const SVElement& rhs) {
	uint8_t carry = 0;	
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint16_t x = opL[i_byte] + rhs[i_byte] + carry;
		carry = (x & (0xFF00)) ? 1 : 0;
		(*this)[i_byte] = static_cast<uint8_t>(x);
	}
	return(*this);
}

SVElement& SVElement::s_add(const SVElement& opL, const int64_t rhs) {
	uint8_t carry = 0;
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte) {
		uint16_t x = 0;
		if(i_byte < 8)  x = opL[i_byte] + (0xFF&(rhs >> 8*i_byte)) + carry;
		else x = opL[i_byte] + ((rhs >= 0) ? 0 : 0xFF) + carry;
		carry = (x&0xFF00)? 1 : 0;
		(*this)[i_byte] = static_cast<uint8_t>(x);
	}
	return (*this);
}


SVElement& SVElement::s_sub(const SVElement& opL, const SVElement& rhs) {
	SVElement twos(rhs);
	twos.twos_complement();
	return(s_add(opL, twos));
}	

SVElement& SVElement::s_sub(const SVElement& opL, const int64_t rhs){
	return(s_add(opL, (~rhs) +1));
}

SVElement& SVElement::s_rsub(const int64_t lhs, const SVElement& opR){
	// this = lhs - opR = (-opR) + lhs (but slow)
	SVElement twos(opR);
	return(s_add(opR, lhs));
}

SVElement& SVElement::s_waddu(const SVElement& opL, const SVElement& rhs) {
	uint8_t carry = 0;
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint8_t _rhs = (i_byte < rhs.width_in_bits_) ? rhs[i_byte] : 0;
		uint8_t _lhs = (i_byte < opL.width_in_bits_) ? opL[i_byte] : 0;
		uint16_t x = _lhs + _rhs + carry;
		carry = (x & (0xFF00)) ? 1 : 0;
		(*this)[i_byte] = static_cast<uint8_t>(x);
	}
	return(*this);
}

SVElement& SVElement::s_waddu(const SVElement& opL, const int64_t rhs) {
	uint8_t carry = 0;
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte) {
		uint8_t _rhs = (i_byte < 8) ? (0xFF&(rhs >> 8*i_byte)) : 0;
		uint8_t _lhs = (i_byte < opL.width_in_bits_) ? opL[i_byte] : 0;
		uint16_t x = _lhs + _rhs + carry;
		carry = (x&0xFF00)? 1 : 0;
		(*this)[i_byte] = static_cast<uint8_t>(x);
	}
	return (*this);
}

SVElement& SVElement::s_wsubu(const SVElement& opL, const SVElement& rhs) {
	SVElement twos(rhs);
	twos.twos_complement();
	return(s_waddu(opL, twos));
}

SVElement& SVElement::s_wsubu(const SVElement& opL, const int64_t rhs) {
	return(s_waddu(opL, (~rhs) +1));
}

SVElement& SVElement::s_wadd(const SVElement& opL, const SVElement& rhs) {
	uint8_t carry = 0;
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		uint8_t _rhs = (i_byte < rhs.width_in_bits_) ? rhs[i_byte] : (rhs[opL.width_in_bits_/8 -1] & 0x80) ? 0xFF : 0x00;
		uint8_t _lhs = (i_byte < opL.width_in_bits_) ? opL[i_byte] : (opL[opL.width_in_bits_/8 -1] & 0x80) ? 0xFF : 0x00;
		uint16_t x = _lhs + _rhs + carry;
		carry = (x & (0xFF00)) ? 1 : 0;
		(*this)[i_byte] = static_cast<uint8_t>(x);
	}
	return(*this);
}

SVElement& SVElement::s_wadd(const SVElement& opL, const int64_t rhs) {
	uint8_t carry = 0;
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte) {
		uint8_t _rhs = (i_byte < 8) ? (0xFF&(rhs >> 8*i_byte)) : ((rhs >= 0) ? 0 : 0xFF);
		uint8_t _lhs = (i_byte < opL.width_in_bits_) ? opL[i_byte] : (opL[opL.width_in_bits_/8 -1] & 0x80) ? 0xFF : 0x00;
		uint16_t x = _lhs + _rhs + carry;
		carry = (x&0xFF00)? 1 : 0;
		(*this)[i_byte] = static_cast<uint8_t>(x);
	}
	return (*this);
}

SVElement& SVElement::s_wsub(const SVElement& opL, const SVElement& rhs) {
	SVElement twos(rhs);
	twos.twos_complement();
	return(s_wadd(opL, twos));
}

SVElement& SVElement::s_wsub(const SVElement& opL, const int64_t rhs) {
	return(s_wadd(opL, (~rhs) +1));
}

SVElement& SVElement::s_and(const SVElement& opL, const SVElement& rhs) {
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		(*this)[i_byte] = opL[i_byte] & rhs[i_byte];
	}
	return(*this);
}
SVElement& SVElement::s_and(const SVElement& opL, const int64_t rhs) {
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		if(i_byte < 8)
			(*this)[i_byte] = opL[i_byte] & (0xFF&(rhs >> 8*i_byte));
		else 
			(*this)[i_byte] = opL[i_byte] & ((rhs >= 0) ? 0 : 0xFF);
	}
	return(*this);
}
SVElement& SVElement::s_or(const SVElement& opL, const SVElement& rhs) {
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		(*this)[i_byte] = opL[i_byte] | rhs[i_byte];
	}
	return(*this);
}
SVElement& SVElement::s_or(const SVElement& opL, const int64_t rhs) {
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		if(i_byte < 8)
			(*this)[i_byte] = opL[i_byte] | (0xFF&(rhs >> 8*i_byte));
		else 
			(*this)[i_byte] = opL[i_byte] | ((rhs >= 0) ? 0 : 0xFF);
	}
	return(*this);
}
SVElement& SVElement::s_xor(const SVElement& opL, const SVElement& rhs) {
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		(*this)[i_byte] = opL[i_byte] ^ rhs[i_byte];
	}
	return(*this);
}
SVElement& SVElement::s_xor(const SVElement& opL, const int64_t rhs) {
	for (size_t i_byte = 0; i_byte < width_in_bits_/8; ++i_byte){
		if(i_byte < 8)
			(*this)[i_byte] = opL[i_byte] ^ (0xFF&(rhs >> 8*i_byte));
		else 
			(*this)[i_byte] = opL[i_byte] ^ ((rhs >= 0) ? 0 : 0xFF);
	}
	return(*this);
}
SVElement& SVElement::s_sll(const SVElement& opL, const SVElement& rhs) {
	if(*this != opL)
		*this = opL;
	size_t shiftamount = get_shiftamount(width_in_bits_, &(rhs[0]));
	while (shiftamount--) {
		arr_shift_left(mem_, width_in_bits_/8);
	}
	return(*this);
}	
SVElement& SVElement::s_sll(const SVElement& opL, const uint64_t rhs) {
	if(*this != opL)
		*this = opL;
	size_t shiftamount = get_shiftamount(width_in_bits_, rhs);
	while (shiftamount--) {
		arr_shift_left(mem_, width_in_bits_/8);
	}
	return(*this);
}
SVElement& SVElement::s_sra(const SVElement& opL, const SVElement& rhs){
	if(*this != opL)
		*this = opL;
	size_t shiftamount = get_shiftamount(width_in_bits_, &(rhs[0]));
	while (shiftamount--) {
		arr_shift_right(mem_, width_in_bits_/8, true);
	}
	return(*this);
}
SVElement& SVElement::s_sra(const SVElement& opL, const uint64_t rhs) {
	if(*this != opL)
		*this = opL;
	size_t shiftamount = get_shiftamount(width_in_bits_, rhs);
	while (shiftamount--) {
		arr_shift_right(mem_, width_in_bits_/8, true);
	}
	return(*this);
}
SVElement& SVElement::s_srl(const SVElement& opL, const SVElement& rhs) {
	if(*this != opL)
		*this = opL;
	size_t shiftamount = get_shiftamount(width_in_bits_, &(rhs[0]));
	while (shiftamount--) {
		arr_shift_right(mem_, width_in_bits_/8, false);
	}
	return(*this);
}
SVElement& SVElement::s_srl(const SVElement& opL, const uint64_t rhs) {
	if(*this != opL)
		*this = opL;
	size_t shiftamount = get_shiftamount(width_in_bits_, rhs);
	while (shiftamount--) {
		arr_shift_right(mem_, width_in_bits_/8, false);
	}
	return(*this);
}

//MUL 12.10
///////////////////////////////////////////////////////////////////////////////////////////
SVElement& SVElement::s_ssmul(const SVElement& opL, const SVElement &rhs){

	int size = (int)width_in_bits_/8;  // vll fehlt *LMUL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	uint8_t* out = new uint8_t [2 * width_in_bits_];
	uint16_t *temp1 = new uint16_t;
	uint8_t *temp2 = new uint8_t;

	// negativ * negativ
	if ((opL[size - 1 ] & 0x80) && (rhs[size - 1] & 0x80))
	{
		for (int i = 0; i < size; i++)
		{
			rhs[i] = ~rhs[i];
			opL[i] = ~opL[i];
		}
		rhs[0] = rhs[0] + 1; //two complement
		opL[0] = opL[0] + 1; //two complement
		
		//same as 2 unsigned positive numbers
		for (int i = 0; i < width_in_bits_ / 8; i++)
		{
			for (int j = 0; j < width_in_bits_ / 8; j++)
			{
				*temp1 = opL[i] * rhs[j];
				out[i + j] = out[i + j] + (uint8_t)(*temp1); // same as & 0x00FF

				*temp2 = (*temp1 >> 8); //upper half going into out[i+j+1]
				out[i + j + 1] = out[i + j + 1] + (uint8_t)(*temp2);
		
			}
		}

	}
	// negativ * positiv
	else if ((opL[size - 1] & 0x80) && !(rhs[size - 1] & 0x80))
	{
			for (int i = 0; i < size; i++)
				{
					opL[i] = ~opL[i];
				}
				opL[0] = opL[0] + 1; //two complement

				//same as 2 unsigned positive numbers
				for (int i = 0; i < width_in_bits_ / 8; i++)
				{
					for (int j = 0; j < width_in_bits_ / 8; j++)
					{
						*temp1 = opL[i] * rhs[j];
						out[i + j] = out[i + j] + (uint8_t)(*temp1); // same as & 0x00FF

						*temp2 = (*temp1 >> 8); //upper half going into out[i+j+1]
						out[i + j + 1] = out[i + j + 1] + (uint8_t)(*temp2);
			
					}
				}
			// Result becomes negative again
			for (int i = 0; i < 2 * size; i++)
			{
				out[i] = ~out[i];
			}
			out[0]  = out[0] + 1;
		}
		// positiv * negativ
		else
		if (!(opL[size - 1] & 0x80) && (rhs[size - 1] & 0x80))
		{
			for (int i = 0; i < size; i++)
			{
				rhs[i] = ~rhs[i];
			}
			rhs[0] = rhs[0] + 1; //two complement
			
			//same as 2 unsigned numbers
			for (int i = 0; i < width_in_bits_ / 8; i++)
			{
				for (int j = 0; j < width_in_bits_ / 8; j++)
				{
					*temp1 = opL[i] * rhs[j];
					out[i + j] = out[i + j] + (uint8_t)(*temp1); // same as & 0x00FF

					*temp2 = (*temp1 >> 8); //upper half going into out[i+j+1]
					out[i + j + 1] = out[i + j + 1] + (uint8_t)(*temp2);
		
				}
			}
			// Result becomes negative again
			for (int i = 0; i < 2 * size; i++)
			{
				out[i] = ~out[i];
			}
				out[0]  = out[0] + 1; 
		}
		else
		{
			// positiv * positiv
			for (int i = 0; i < width_in_bits_ / 8; i++)
			{
				for (int j = 0; j < width_in_bits_ / 8; j++)
				{
					*temp1 = opL[i] * rhs[j];
					out[i + j] = out[i + j] + (uint8_t)(*temp1); // same as & 0x00FF

					*temp2 = (*temp1 >> 8); //upper half going into out[i+j+1]
					out[i + j + 1] = out[i + j + 1] + (uint8_t)(*temp2);
		
				}
			}
		}
	// returning lower half of SIgned*Signed MUL
	for (size_t i = 0; i <  size; i++)
	{
		(*this)[i] = out[i];
	};

	delete temp1;
	delete temp2;
	delete[] out;

	return (*this);
} 
SVElement& SVElement::s_ssmul(const SVElement& opL, const int64_t rhs){

	int size = width_in_bits_/8;  // vll fehlt *LMUL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	uint8_t* out = new uint8_t [2 * width_in_bits_];
	uint16_t *temp1 = new uint16_t;
	uint8_t *temp2 = new uint8_t;
	// Changing 64bit signed into 8bit Array
	uint8_t* rhsarray = new uint8_t [8];
	for (int i = 0; i < 8; i++)
	{
		rhsarray[i] = (u_int8_t)(rhs >> i*8)
	}

	// negativ * negativ
	if ((opL[size - 1 ] & 0x80) && (rhsarray[7] & 0x80))
	{
		for (int i = 0; i < size; i++)
		{
			rhsarray[i] = ~rhsarray[i];
			opL[i] = ~opL[i];
		}
		rhsarray[0] = rhsarray[0] + 1; //two complement
		opL[0] = opL[0] + 1; //two complement
		
		//same as 2 unsigned positive numbers
		for (int i = 0; i < width_in_bits_ / 8; i++)
		{
			for (int j = 0; j < 7; j++)
			{
				*temp1 = opL[i] * rhsarray[j];
				out[i + j] = out[i + j] + (uint8_t)(*temp1); // same as & 0x00FF

				*temp2 = (*temp1 >> 8); //upper half going into out[i+j+1]
				out[i + j + 1] = out[i + j + 1] + (uint8_t)(*temp2);
	
			}
		}

	}
	// negativ * positiv
	else if ((opL[size - 1] & 0x80) && !(rhsarray[7] & 0x80))
	{
			for (int i = 0; i < size; i++)
				{
					opL[i] = ~opL[i];
				}
				opL[0] = opL[0] + 1; //two complement

				//same as 2 unsigned positive numbers
				for (int i = 0; i < width_in_bits_ / 8; i++)
				{
					for (int j = 0; j < 7; j++)
					{
						*temp1 = opL[i] * rhsarray[j];
						out[i + j] = out[i + j] + (uint8_t)(*temp1); // same as & 0x00FF

						*temp2 = (*temp1 >> 8); //upper half going into out[i+j+1]
						out[i + j + 1] = out[i + j + 1] + (uint8_t)(*temp2);
			
					}
				}
			// Result becomes negative again
			for (int i = 0; i < 2 * size; i++)
			{
				out[i] = ~out[i];
			}
			out[0]  = out[0] + 1;
	}
		// positiv * negativ
		else
		if (!(opL[size - 1] & 0x80) && (rhsarray[7] & 0x80))
		{
			for (int i = 0; i < 7; i++)
			{
				rhsarray[i] = ~rhsarray[i];
			}
			rhs[0] = rhs[0] + 1; //two complement
			
			//same as 2 unsigned numbers
			for (int i = 0; i < width_in_bits_ / 8; i++)
			{
				for (int j = 0; j < 7; j++)
				{
					*temp1 = opL[i] * rhsarray[j];
					out[i + j] = out[i + j] + (uint8_t)(*temp1); // same as & 0x00FF

					*temp2 = (*temp1 >> 8); //upper half going into out[i+j+1]
					out[i + j + 1] = out[i + j + 1] + (uint8_t)(*temp2);
		
				}
			}
			// Result becomes negative again
			for (int i = 0; i < 2 * size; i++)
			{
				out[i] = ~out[i];
			}
				out[0]  = out[0] + 1; 
		}
		else
		{
			// positiv * positiv
			for (int i = 0; i < width_in_bits_ / 8; i++)
			{
				for (int j = 0; j < 7; j++)
				{
					*temp1 = opL[i] * rhsarray[j];
					out[i + j] = out[i + j] + (uint8_t)(*temp1); // same as & 0x00FF

					*temp2 = (*temp1 >> 8); //upper half going into out[i+j+1]
					out[i + j + 1] = out[i + j + 1] + (uint8_t)(*temp2);
		
				}
			}
		}
	// returning lower half of SIgned*Signed MUL
	for (int i = 0; i <  size; i++)
	{
		(*this)[i] = out[i];
	}

	delete temp1;
	delete temp2;
	delete[] out;
	delete[] rhsarray;
	return (*this);

}
