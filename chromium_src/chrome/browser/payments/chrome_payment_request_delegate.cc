/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/browser/brave_rewards/checkout_dialog.h"
#include "components/payments/content/payment_request.h"
#include "components/payments/core/payment_app.h"
#include "content/public/browser/web_contents.h"

using brave_rewards::CheckoutDialogController;
using brave_rewards::ShowCheckoutDialog;
using payments::PaymentApp;

namespace {

const char kBat[] = "bat";

class BATPaymentApp : public PaymentApp {
 public:
  BATPaymentApp(
      payments::PaymentRequest* request,
      base::WeakPtr<CheckoutDialogController> controller)
      : payments::PaymentApp(0, PaymentApp::Type::SERVICE_WORKER_APP),
        request_(request),
        controller_(controller),
        weak_factory_(this) {
    DCHECK(request);
    DCHECK(controller);

    app_method_names_.insert(kBat);

    controller->SetOnDialogClosedCallback(base::BindOnce(
        &BATPaymentApp::OnDialogClosed,
        weak_factory_.GetWeakPtr()));

    controller->SetOnPaymentReadyCallback(base::BindOnce(
        &BATPaymentApp::OnPaymentReady,
        weak_factory_.GetWeakPtr()));
  }

  ~BATPaymentApp() override {}

  void InvokePaymentApp(Delegate* delegate) override {}

  bool IsCompleteForPayment() const override {
    return true;
  }

  uint32_t GetCompletenessScore() const override {
    return 0;
  }

  bool CanPreselect() const override {
    return false;
  }

  base::string16 GetMissingInfoLabel() const override {
    return base::string16();
  }

  bool IsValidForCanMakePayment() const override {
    return false;
  }

  void RecordUse() override {}

  bool NeedsInstallation() const override {
    return false;
  }

  base::string16 GetLabel() const override {
    return base::ASCIIToUTF16(kBat);
  }

  base::string16 GetSublabel() const override {
    return base::string16();
  }

  bool IsValidForModifier(
      const std::string& method,
      bool supported_networks_specified,
      const std::set<std::string>& supported_networks) const override {
    return false;
  }

  base::WeakPtr<PaymentApp> AsWeakPtr() override {
    return weak_factory_.GetWeakPtr();
  }

  gfx::ImageSkia icon_image_skia() const override {
    return icon_;
  }

  bool HandlesShippingAddress() const override {
    return false;
  }

  bool HandlesPayerName() const override {
    return false;
  }

  bool HandlesPayerEmail() const override {
    return false;
  }

  bool HandlesPayerPhone() const override {
    return false;
  }

 private:
  void OnDialogClosed(bool payment_fulfilled) {
    request_->UserCancelled();
  }

  void OnPaymentReady() {
    auto response = payments::mojom::PaymentResponse::New();
    response->method_name = kBat;
    response->stringified_details = "{}";

    auto payer = payments::mojom::PayerDetail::New();
    response->payer = std::move(payer);

    request_->Pay();
    request_->OnPaymentResponseAvailable(std::move(response));
  }

  gfx::ImageSkia icon_;
  payments::PaymentRequest* request_;
  base::WeakPtr<CheckoutDialogController> controller_;
  base::WeakPtrFactory<BATPaymentApp> weak_factory_;
};

bool BraveShowPaymentDialog(
    content::WebContents* initiator,
    payments::PaymentRequest* request) {
  auto* spec = request->spec();
  if (!spec || spec->stringified_method_data().count("bat") == 0) {
    return false;
  }

  double total_amount = 0;
  base::StringToDouble(spec->details().total->amount->value, &total_amount);

  auto controller = ShowCheckoutDialog(initiator, {
    spec->details().total->label,
    total_amount,
    {} // TODO(zenparsing): Add sku/description items
  });

  auto app = std::make_unique<BATPaymentApp>(request, controller);
  request->state()->OnPaymentAppCreated(std::move(app));
  return true;
}

}  // namespace

#define BRAVE_SHOW_BAT_PAYMENT_UI \
  if (BraveShowPaymentDialog(web_contents_, request)) \
    return;

#include "../../../../../chrome/browser/payments/chrome_payment_request_delegate.cc"

#undef BRAVE_SHOW_BAT_PAYMENT_UI
