#include "midieat.c"
#include "route.c"

static const void *
extension_data (const char * uri)
{
  return NULL;
}

#define M_DESCRIPTOR(ID) \
  static const LV2_Descriptor descriptor_##ID = { \
    PLB_URI #ID, m_instantiate, m_connect_port, \
    NULL,        m_run,         NULL, \
    m_cleanup,   extension_data \
  };

#define A_DESCRIPTOR(ID) \
  static const LV2_Descriptor descriptor_##ID = { \
    PLB_URI #ID, a_instantiate, a_connect_port, \
    NULL,        a_run,         NULL, \
    a_cleanup,   extension_data \
  };

M_DESCRIPTOR (eat1)
M_DESCRIPTOR (eat2)
M_DESCRIPTOR (eat3)
M_DESCRIPTOR (eat4)

M_DESCRIPTOR (gen1)
M_DESCRIPTOR (gen2)
M_DESCRIPTOR (gen3)
M_DESCRIPTOR (gen4)

A_DESCRIPTOR (route_1_2)
A_DESCRIPTOR (route_1_3)
A_DESCRIPTOR (route_1_4)

A_DESCRIPTOR (route_2_1)
A_DESCRIPTOR (route_2_2)
A_DESCRIPTOR (route_2_3)
A_DESCRIPTOR (route_2_4)

A_DESCRIPTOR (route_3_1)
A_DESCRIPTOR (route_3_2)
A_DESCRIPTOR (route_3_3)
A_DESCRIPTOR (route_3_4)

A_DESCRIPTOR (route_4_1)
A_DESCRIPTOR (route_4_2)
A_DESCRIPTOR (route_4_3)
A_DESCRIPTOR (route_4_4)

#undef LV2_SYMBOL_EXPORT
#ifdef _WIN32
#  define LV2_SYMBOL_EXPORT __declspec(dllexport)
#else
#  define LV2_SYMBOL_EXPORT \
    __attribute__ ((visibility ("default")))
#endif
LV2_SYMBOL_EXPORT
const LV2_Descriptor *
lv2_descriptor (uint32_t index)
{
  switch (index)
    {
    case 0:
      return &descriptor_eat1;
    case 1:
      return &descriptor_eat2;
    case 2:
      return &descriptor_eat3;
    case 3:
      return &descriptor_eat4;

    case 4:
      return &descriptor_gen1;
    case 5:
      return &descriptor_gen2;
    case 6:
      return &descriptor_gen3;
    case 7:
      return &descriptor_gen4;

    case 8:
      return &descriptor_route_1_2;
    case 9:
      return &descriptor_route_1_3;
    case 10:
      return &descriptor_route_1_4;

    case 11:
      return &descriptor_route_2_1;
    case 12:
      return &descriptor_route_2_2;
    case 13:
      return &descriptor_route_2_3;
    case 14:
      return &descriptor_route_2_4;

    case 15:
      return &descriptor_route_3_1;
    case 16:
      return &descriptor_route_3_2;
    case 17:
      return &descriptor_route_3_3;
    case 18:
      return &descriptor_route_3_4;

    case 19:
      return &descriptor_route_4_1;
    case 20:
      return &descriptor_route_4_2;
    case 21:
      return &descriptor_route_4_3;
    case 22:
      return &descriptor_route_4_4;

    default:
      return NULL;
    }
}
