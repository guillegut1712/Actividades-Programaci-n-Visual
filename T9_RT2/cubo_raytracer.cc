#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include <random>
#include <limits>


using std::shared_ptr;
using std::make_shared;

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

inline double random_double() {
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
}

inline double random_double(double min, double max) {
    return min + (max - min) * random_double();
}

inline int random_int(int min, int max) {
    return static_cast<int>(random_double(min, max + 1));
}

inline double clamp(double x, double min, double max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}


class vec3 {
public:

    double e[3];

    vec3() : e{ 0,0,0 } {}
    vec3(double e0, double e1, double e2) : e{ e0, e1, e2 } {}

    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }

    vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
    double operator[](int i) const { return e[i]; }
    double& operator[](int i) { return e[i]; }

    vec3& operator+=(const vec3& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    vec3& operator*=(const double t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    vec3& operator/=(const double t) {
        return *this *= 1 / t;
    }

    double length() const {
        return std::sqrt(length_squared());
    }

    double length_squared() const {
        return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
    }

    bool near_zero() const {
        const auto s = 1e-8;
        return (fabs(e[0]) < s) && (fabs(e[1]) < s) && (fabs(e[2]) < s);
    }

    static vec3 random() {
        return vec3(random_double(), random_double(), random_double());
    }

    static vec3 random(double min, double max) {
        return vec3(random_double(min, max), random_double(min, max), random_double(min, max));
    }
};


using point3 = vec3; 
using color = vec3; 

// aquí van todas las funciones para los vectores
inline std::ostream& operator<<(std::ostream& out, const vec3& v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}
inline vec3 operator+(const vec3& u, const vec3& v) {
    return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}
inline vec3 operator-(const vec3& u, const vec3& v) {
    return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}
inline vec3 operator*(const vec3& u, const vec3& v) {
    return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}
inline vec3 operator*(double t, const vec3& v) {
    return vec3(t * v.e[0], t * v.e[1], t * v.e[2]);
}
inline vec3 operator*(const vec3& v, double t) {
    return t * v;
}
inline vec3 operator/(vec3 v, double t) {
    return (1 / t) * v;
}
inline double dot(const vec3& u, const vec3& v) {
    return u.e[0] * v.e[0]
        + u.e[1] * v.e[1]
        + u.e[2] * v.e[2];
}
inline vec3 cross(const vec3& u, const vec3& v) {
    return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
        u.e[2] * v.e[0] - u.e[0] * v.e[2],
        u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}
inline vec3 unit_vector(const vec3& v) {
    return v / v.length();
}
inline vec3 random_in_unit_disk() {
    while (true) {
        auto p = vec3(random_double(-1, 1), random_double(-1, 1), 0);
        if (p.length_squared() < 1)
            return p;
    }
}
inline vec3 random_in_unit_sphere() {
    while (true) {
        auto p = vec3::random(-1, 1);
        if (p.length_squared() < 1)
            return p;
    }
}
inline vec3 random_unit_vector() {
    return unit_vector(random_in_unit_sphere());
}
inline vec3 random_on_hemisphere(const vec3& normal) {
    vec3 on_unit_sphere = random_unit_vector();
    if (dot(on_unit_sphere, normal) > 0.0)
        return on_unit_sphere;
    else
        return -on_unit_sphere;
}
inline vec3 reflect(const vec3& v, const vec3& n) {
    return v - 2 * dot(v, n) * n;
}
inline vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat) {
    auto cos_theta = fmin(dot(-uv, n), 1.0);
    vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
    vec3 r_out_parallel = -sqrt(fabs(1.0 - r_out_perp.length_squared())) * n;
    return r_out_perp + r_out_parallel;
}




// esta es la clase para hacer rotar los vectores
class rotation {
public:
    rotation() : rotation_matrix{ 1,0,0,0,1,0,0,0,1 } {}

    rotation(double angle_x, double angle_y, double angle_z) {
        // se crean matrices de rotación individuales
        double matrix_x[9] = {
            1, 0, 0,
            0, cos(angle_x), -sin(angle_x),
            0, sin(angle_x), cos(angle_x)
        };

        double matrix_y[9] = {
            cos(angle_y), 0, sin(angle_y),
            0, 1, 0,
            -sin(angle_y), 0, cos(angle_y)
        };

        double matrix_z[9] = {
            cos(angle_z), -sin(angle_z), 0,
            sin(angle_z), cos(angle_z), 0,
            0, 0, 1
        };

        // aqui multiplicamos las matrices para obtener la rotación completa
        double temp[9];
        matrix_multiply(matrix_y, matrix_x, temp);
        matrix_multiply(matrix_z, temp, rotation_matrix);
    }

    vec3 rotate(const vec3& v) const {
        return vec3(
            rotation_matrix[0] * v.x() + rotation_matrix[1] * v.y() + rotation_matrix[2] * v.z(),
            rotation_matrix[3] * v.x() + rotation_matrix[4] * v.y() + rotation_matrix[5] * v.z(),
            rotation_matrix[6] * v.x() + rotation_matrix[7] * v.y() + rotation_matrix[8] * v.z()
        );
    }

private:
    double rotation_matrix[9];

    void matrix_multiply(const double A[9], const double B[9], double C[9]) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                C[i * 3 + j] = 0;
                for (int k = 0; k < 3; k++) {
                    C[i * 3 + j] += A[i * 3 + k] * B[k * 3 + j];
                }
            }
        }
    }
};



// esta es la clase del intervalo
class interval {
public:
    double min, max;

    interval() : min(+infinity), max(-infinity) {}
    interval(double _min, double _max) : min(_min), max(_max) {}

    bool contains(double x) const {
        return min <= x && x <= max;
    }

    bool surrounds(double x) const {
        return min < x && x < max;
    }

    double clamp(double x) const {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    static const interval empty, universe;
};

const interval interval::empty = interval(+infinity, -infinity);
const interval interval::universe = interval(-infinity, +infinity);



class ray {
public:
    ray() {}
    ray(const point3& origin, const vec3& direction)
        : orig(origin), dir(direction) {
    }

    point3 origin() const { return orig; }
    vec3 direction() const { return dir; }

    point3 at(double t) const {
        return orig + t * dir;
    }

public:
    point3 orig;
    vec3 dir;
};




void write_color(std::ostream& out, color pixel_color, int samples_per_pixel) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();
    auto scale = 1.0 / samples_per_pixel;
    r = sqrt(scale * r);
    g = sqrt(scale * g);
    b = sqrt(scale * b);

    out << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
        << static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
        << static_cast<int>(256 * clamp(b, 0.0, 0.999)) << '\n';
}


class material;

struct hit_record {
    point3 p;
    vec3 normal;
    shared_ptr<material> mat_ptr;
    double t;
    bool front_face;

    inline void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};


class hittable {
public:
    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
};

// esta es la clase del cubo que debe estar rotado
class rotated_box : public hittable {
public:
    rotated_box() {}
    rotated_box(point3 center, double size, shared_ptr<material> m, rotation r)
        : center(center), half_size(size / 2), mat_ptr(m), rot(r) {
    }

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        point3 origin = r.origin() - center;
        vec3 dir_inv = vec3(-rot.rotate(-r.direction()).x(), -rot.rotate(-r.direction()).y(), -rot.rotate(-r.direction()).z());
        point3 orig_inv = vec3(-rot.rotate(-origin).x(), -rot.rotate(-origin).y(), -rot.rotate(-origin).z());
        vec3 dir = -dir_inv;
        point3 orig = -orig_inv;

        double t_min = ray_t.min;
        double t_max = ray_t.max;

        for (int a = 0; a < 3; a++) {
            auto invD = 1.0f / dir[a];
            auto t0 = (-half_size - orig[a]) * invD;
            auto t1 = (half_size - orig[a]) * invD;

            if (invD < 0.0f)
                std::swap(t0, t1);

            t_min = t0 > t_min ? t0 : t_min;
            t_max = t1 < t_max ? t1 : t_max;

            if (t_max <= t_min)
                return false;
        }

  
        rec.t = t_min;
        rec.p = r.at(rec.t);
        point3 local_p = rot.rotate(rec.p - center);
        vec3 outward_normal;

        double eps = 1e-8;
        if (std::abs(local_p.x() - half_size) < eps)
            outward_normal = rot.rotate(vec3(1, 0, 0));
        else if (std::abs(local_p.x() + half_size) < eps)
            outward_normal = rot.rotate(vec3(-1, 0, 0));
        else if (std::abs(local_p.y() - half_size) < eps)
            outward_normal = rot.rotate(vec3(0, 1, 0));
        else if (std::abs(local_p.y() + half_size) < eps)
            outward_normal = rot.rotate(vec3(0, -1, 0));
        else if (std::abs(local_p.z() - half_size) < eps)
            outward_normal = rot.rotate(vec3(0, 0, 1));
        else
            outward_normal = rot.rotate(vec3(0, 0, -1));

        rec.set_face_normal(r, outward_normal);
        rec.mat_ptr = mat_ptr;

        return true;
    }

private:
    point3 center;
    double half_size;
    shared_ptr<material> mat_ptr;
    rotation rot;
};

// esta es la clase de esfera
class sphere : public hittable {
public:
    sphere() {}
    sphere(point3 cen, double r, shared_ptr<material> m)
        : center(cen), radius(r), mat_ptr(m) {
    };

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        vec3 oc = r.origin() - center;
        auto a = r.direction().length_squared();
        auto half_b = dot(oc, r.direction());
        auto c = oc.length_squared() - radius * radius;
        auto discriminant = half_b * half_b - a * c;
        if (discriminant < 0) return false;
        auto sqrtd = sqrt(discriminant);
        auto root = (-half_b - sqrtd) / a;
        if (!ray_t.surrounds(root)) {
            root = (-half_b + sqrtd) / a;
            if (!ray_t.surrounds(root))
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.mat_ptr = mat_ptr;

        return true;
    }

private:
    point3 center;
    double radius;
    shared_ptr<material> mat_ptr;
};


class hittable_list : public hittable {
public:
    std::vector<shared_ptr<hittable>> objects; 

    hittable_list() {}
    hittable_list(shared_ptr<hittable> object) { add(object); }

    void clear() { objects.clear(); }
    void add(shared_ptr<hittable> object) { objects.push_back(object); }

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        hit_record temp_rec;
        bool hit_anything = false;
        auto closest_so_far = ray_t.max;

        for (const auto& object : objects) {
            if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }
};

// esta es la clase de material
class material {
public:
    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
    ) const = 0;
};

// esta es la clase del material lambertiano de difusion
class lambertian : public material {
public:
    lambertian(const color& a) : albedo(a) {}

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
    ) const override {
        auto scatter_direction = rec.normal + random_unit_vector();

        if (scatter_direction.near_zero())
            scatter_direction = rec.normal;

        scattered = ray(rec.p, scatter_direction);
        attenuation = albedo;
        return true;
    }

public:
    color albedo;
};

// este es es el material de metal
class metal : public material {
public:
    metal(const color& a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
    ) const override {
        vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        scattered = ray(rec.p, reflected + fuzz * random_in_unit_sphere());
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
    }

public:
    color albedo;
    double fuzz;
};

// este es el material de vidrio
class dielectric : public material {
public:
    dielectric(double index_of_refraction) : ir(index_of_refraction) {}

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
    ) const override {
        attenuation = color(1.0, 1.0, 1.0);
        double refraction_ratio = rec.front_face ? (1.0 / ir) : ir;

        vec3 unit_direction = unit_vector(r_in.direction());
        double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, refraction_ratio);

        scattered = ray(rec.p, direction);
        return true;
    }

public:
    double ir;

private:
    static double reflectance(double cosine, double ref_idx) {
        auto r0 = (1 - ref_idx) / (1 + ref_idx);
        r0 = r0 * r0;
        return r0 + (1 - r0) * pow((1 - cosine), 5);
    }
};

// esta es la clase para la camara
class camera {
public:
    double aspect_ratio = 1.0;
    int    image_width = 100;
    int    samples_per_pixel = 10;
    int    max_depth = 10;

    double vfov = 90;
    point3 lookfrom = point3(0, 0, 0);
    point3 lookat = point3(0, 0, -1);
    vec3   vup = vec3(0, 1, 0);

    double defocus_angle = 0;
    double focus_dist = 1;

    void render(const hittable& world) {
        initialize();

        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        for (int j = 0; j < image_height; ++j) {
            std::cerr << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            for (int i = 0; i < image_width; ++i) {
                color pixel_color(0, 0, 0);
                for (int s = 0; s < samples_per_pixel; ++s) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }
                write_color(std::cout, pixel_color, samples_per_pixel);
            }
        }

        std::cerr << "\rDone.                 \n";
    }

private:
    int    image_height;
    point3 center;
    point3 pixel00_loc;
    vec3   pixel_delta_u;
    vec3   pixel_delta_v;
    vec3   u, v, w;
    vec3   defocus_disk_u;
    vec3   defocus_disk_v;

    void initialize() {
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;
        center = lookfrom;

        auto theta = degrees_to_radians(vfov);
        auto h = tan(theta / 2);
        auto viewport_height = 2 * h * focus_dist;
        auto viewport_width = viewport_height * static_cast<double>(image_width) / image_height;

        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        vec3 viewport_u = viewport_width * u;
        vec3 viewport_v = viewport_height * -v;

        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        auto viewport_upper_left = center - (focus_dist * w) - viewport_u / 2 - viewport_v / 2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        auto defocus_radius = focus_dist * tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    ray get_ray(int i, int j) const {
        auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
        auto pixel_sample = pixel_center + pixel_sample_square();
        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin;
        return ray(ray_origin, ray_direction);
    }

    vec3 pixel_sample_square() const {
        auto px = -0.5 + random_double();
        auto py = -0.5 + random_double();
        return (px * pixel_delta_u) + (py * pixel_delta_v);
    }

    point3 defocus_disk_sample() const {
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }

    color ray_color(const ray& r, int depth, const hittable& world) const {
        if (depth <= 0)
            return color(0, 0, 0);

        hit_record rec;

        if (world.hit(r, interval(0.001, infinity), rec)) {
            ray scattered;
            color attenuation;
            if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
                return attenuation * ray_color(scattered, depth - 1, world);
            return color(0, 0, 0);
        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
    }
};




// esta es la funcion main 
int main() {
    
    hittable_list world;

    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));

    // aquí rotamos todos los cubos para poder ver 2 caras desde la camara
    rotation cube_rotation(0, degrees_to_radians(45), 0);

    // aquí creamos los cubos pequeños dispersos en diferentes areas para que sean visibles desde los cubos grandes
    for (int i = 0; i < 30; i++) {

        double x, z;
        int zone = i % 5;

        if (zone == 0) {
            // esta es la zona importante para que sean evidentes los cubos reflejados entre la camar ay el promer cubo reflexivo
            x = random_double(2, 9);
            z = random_double(1, 2.5);
        }
        else if (zone == 1) {
            x = random_double(-2, 2);
            z = random_double(1.5, 5);
        }
        else if (zone == 2) {
            x = random_double(-6, -3);
            z = random_double(1, 3);
        }
        else if (zone == 3) {
            x = random_double(3, 6);
            z = random_double(1, 3);
        }
        else {
            x = random_double(-3, 3);
            z = random_double(-0.5, 3);
        }

        double y = 0.2;

        // aca le damos colores llamativos a los cubos pequeños que seran reflejados
        color cube_color;
        int color_choice = i % 6;
        switch (color_choice) {
        case 0: cube_color = color(1.0, 0.2, 0.2); break;
        case 1: cube_color = color(0.2, 1.0, 0.2); break;
        case 2: cube_color = color(0.2, 0.2, 1.0); break;
        case 3: cube_color = color(1.0, 1.0, 0.2); break;
        case 4: cube_color = color(1.0, 0.2, 1.0); break;
        case 5: cube_color = color(0.2, 1.0, 1.0); break;
        }

        auto cube_material = make_shared<lambertian>(cube_color);
        double size = random_double(0.25, 0.4);
        world.add(make_shared<rotated_box>(point3(x, y, z), size, cube_material, cube_rotation));
    }

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<rotated_box>(point3(4, 1, 0), 2.0, material3, cube_rotation));

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<rotated_box>(point3(0, 1, 0), 2.0, material1, cube_rotation));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<rotated_box>(point3(-4, 1, 0), 2.0, material2, cube_rotation));

    // aquí se prepara la camara
    camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;             
    cam.samples_per_pixel = 20;    
    cam.max_depth = 10;                
    cam.vfov = 20;
    cam.lookfrom = point3(13, 2, 3);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);
    cam.defocus_angle = 0.6;
    cam.focus_dist = 10.0;
    cam.render(world);

    return 0;
}