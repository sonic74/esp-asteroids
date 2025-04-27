//
//  Controls.hpp
//  AsteroidsTestBed
//
//  Created by Chris Greening on 08/01/2021.
//

#ifndef Controls_hpp
#define Controls_hpp

#include <stdint.h>

class Controls {
public:
    virtual bool is_firing() = 0;
    virtual bool is_shielding()=0;
    virtual float get_thrust() = 0;
    virtual float get_direction() = 0;
    virtual void shake(uint8_t timeActive=50) {}
    virtual int get_function_key()=0;
};

#endif /* Controls_hpp */
