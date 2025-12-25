#define EPS 0.001
#define PI 3.14159265

struct v_info
{
    vec3 uv;
    vec3 w_normal;
    vec3 w_ray;
    vec3 w_tangent;
    vec3 w_bitangent;

    vec3 m_tangent;
    vec3 m_bitangent;
};

float saturate(in float val)
{
    return clamp(val, 0.0, 1.0);
}

/*
float pow(in float num, in int t)
{
    if (t == 0)
    {
        return 1.0;
    }

    float res = num;

    if (t < 0)
    {
        for (int i = 0; i > t + 1; i--)
        {
            res *= num;
        }

        return 1.0 / res;
    }

    if (t > 0)
    {
        for (int i = 0; i < t - 1; i++)
        {
            res *= num;
        }
    }

    return res;
}
*/
