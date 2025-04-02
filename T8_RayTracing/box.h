#ifndef BOX_H
#define BOX_H

#include "rtweekend.h"
#include "hittable.h"
#include "vec3.h"
#include "ray.h"
#include "interval.h"
#include <memory>

using std::shared_ptr;
using std::make_shared;

class material;  // Declaración anticipada

class box : public hittable {
public:
    box() {}
    box(const point3& p0, const point3& p1, shared_ptr<material> m);

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override;

private:
    point3 box_min; // Punto mínimo (esquina inferior izquierda)
    point3 box_max; // Punto máximo (esquina superior derecha)
    shared_ptr<material> mat_ptr;
};

#endif