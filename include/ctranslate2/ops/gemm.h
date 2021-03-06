#pragma once

#include "op.h"

namespace ctranslate2 {
  namespace ops {

    class Gemm : public Op {
    public:
      Gemm(float alpha = 1, float beta = 1, bool trans_a = false, bool trans_b = false);

      void operator()(const std::vector<StorageView*>& inputs,
                      std::vector<StorageView*>& outputs) const override;
      void operator()(const StorageView& a,
                      const StorageView& b,
                      StorageView& c) const;
      void operator()(const StorageView& a,
                      const StorageView& b,
                      const StorageView& c,
                      StorageView& y) const;

    private:
      void compute(const StorageView& a,
                   const StorageView& b,
                   const StorageView* c,
                   StorageView& y) const;

      float _alpha;
      float _beta;
      bool _trans_a;
      bool _trans_b;
    };

  }
}
