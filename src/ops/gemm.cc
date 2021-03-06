#include "ctranslate2/ops/gemm.h"

#include "../device_dispatch.h"

namespace ctranslate2 {
  namespace ops {

    template <Device D, typename In, typename Out>
    static void run_gemm(const StorageView& a, const StorageView& b, const StorageView* c,
                         bool transpose_a, bool transpose_b,
                         dim_t m, dim_t n, dim_t k,
                         float alpha, float beta,
                         StorageView& y) {
      const In* a_data = a.data<In>();
      const In* b_data = b.data<In>();
      Out* y_data = y.data<Out>();

      if (beta != 0 && c) {
        const Out* c_data = c->data<Out>();
        if (c->size() == y.size()) {
          primitives<D>::copy(c_data, y_data, y.size());
        } else if (c->size() == n) {
          for (dim_t i = 0; i < m; ++i)
            primitives<D>::copy(c_data, y_data + i * n, n);
        } else {
          throw std::invalid_argument("c has invalid size");
        }
      }

      primitives<D>::gemm(a_data, b_data,
                          transpose_a, transpose_b,
                          m, n, k,
                          alpha, beta,
                          y_data);
    }


    Gemm::Gemm(float alpha, float beta, bool trans_a, bool trans_b)
      : _alpha(alpha)
      , _beta(beta)
      , _trans_a(trans_a)
      , _trans_b(trans_b) {
    }

    void Gemm::operator()(const std::vector<StorageView*>& inputs,
                          std::vector<StorageView*>& outputs) const {
      if (inputs.size() == 2)
        operator()(*inputs[0], *inputs[1], *outputs[0]);
      else
        operator()(*inputs[0], *inputs[1], *inputs[2], *outputs[0]);
    }

    void Gemm::operator()(const StorageView& a,
                          const StorageView& b,
                          const StorageView& c,
                          StorageView& y) const {
      compute(a, b, &c, y);
    }

    void Gemm::operator()(const StorageView& a,
                          const StorageView& b,
                          StorageView& c) const {
      compute(a, b, nullptr, c);
    }

    void Gemm::compute(const StorageView& a,
                       const StorageView& b,
                       const StorageView* c,
                       StorageView& y) const {
      PROFILE("Gemm");

      const dim_t k = a.dim(_trans_a ? -2 : -1);
      const dim_t n = b.dim(_trans_b ? -2 : -1);
      const dim_t m = a.size() / k;  // Collapse leading dimensions.

      Shape output_shape(a.shape());
      output_shape[output_shape.size() - 1] = n;
      y.resize(output_shape);

      switch (a.dtype()) {
      case DataType::DT_INT8:
        DEVICE_DISPATCH(a.device(),
                        (run_gemm<D, int8_t, int32_t>(a, b, c,
                                                      _trans_a, _trans_b,
                                                      m, n, k,
                                                      _alpha, _beta,
                                                      y)));
        break;

      case DataType::DT_INT16:
        if (a.device() != Device::CPU)
          throw std::invalid_argument("INT16 GEMM is only supported on CPU");
        run_gemm<Device::CPU, int16_t, int32_t>(a, b, c,
                                                _trans_a, _trans_b,
                                                m, n, k,
                                                _alpha, _beta,
                                                y);
        break;

      case DataType::DT_FLOAT:
        DEVICE_DISPATCH(a.device(),
                        (run_gemm<D, float, float>(a, b, c,
                                                   _trans_a, _trans_b,
                                                   m, n, k,
                                                   _alpha, _beta,
                                                   y)));
        break;

      default:
        throw std::invalid_argument("unsupported compute type " + dtype_name(a.dtype()));
      }
    }

  }
}
