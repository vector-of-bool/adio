#ifndef ADIO_CONFIG_COMMON_HPP_INCLUDED
#define ADIO_CONFIG_COMMON_HPP_INCLUDED

#define ADIO_ABI_INNER_NAMESPACE                                                                   \
    ADIO_PASTE(abi_,                                                                               \
               ADIO_PASTE(ADIO_ABI_VERSION,                                                        \
                          ADIO_PASTE(_,                                                            \
                                     ADIO_PASTE(ADIO_BACKEND_NAMESPACE,                            \
                                                ADIO_PASTE(_, ADIO_CONFIG)))))

#define ADIO_NAMESPACE_BEGIN                                                                       \
    namespace adio {                                                                               \
    inline namespace ADIO_ABI_INNER_NAMESPACE [[gnu::abi_tag(ADIO_STR(ADIO_ABI_VERSION),           \
                                                             ADIO_STR(ADIO_ABI_BACKEND),           \
                                                             ADIO_STR(ADIO_CONFIG))]] {

#define ADIO_NAMESPACE_END                                                                         \
    }                                                                                              \
    }  // namespace adio

#define ADIO_NAMESPACE adio::ADIO_ABI_INNER_NAMESPACE

#endif  // ADIO_CONFIG_COMMON_HPP_INCLUDED
