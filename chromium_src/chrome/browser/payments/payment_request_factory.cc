/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "brave/browser/brave_rewards/checkout_dialog.h"
#include "chrome/browser/payments/chrome_payment_request_delegate.cc"
#include "components/payments/content/payment_request.h"
#include "content/public/browser/web_contents.h"

namespace {

constexpr char kBat[] = "bat";

class BATPaymentApp : public payments::PaymentApp {
 public:
  BATPaymentApp()
      : PaymentApp(0, payments::PaymentApp::Type::SERVICE_WORKER_APP),
        weak_factory_(this) {
    app_method_names_.insert(kBat);
  }

  ~BATPaymentApp() override {}

  void InvokePaymentApp(Delegate* delegate) override {
    delegate->OnInstrumentDetailsReady(kBat, details_, payer_data_);
  }

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

  base::WeakPtr<payments::PaymentApp> AsWeakPtr() override {
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
  friend class BravePaymentRequestDelegate;

  gfx::ImageSkia icon_;
  std::string details_ = "{}";
  payments::PayerData payer_data_;
  base::WeakPtrFactory<BATPaymentApp> weak_factory_;
};

class BravePaymentRequestDelegate :
    public payments::ChromePaymentRequestDelegate {
 public:
  explicit BravePaymentRequestDelegate(content::WebContents* web_contents)
      : ChromePaymentRequestDelegate(web_contents),
        web_contents_(web_contents),
        weak_factory_(this) {}

  ~BravePaymentRequestDelegate() override {}

  void ShowDialog(payments::PaymentRequest* request) override {
    using payments::ChromePaymentRequestDelegate;
    using payments::PaymentRequestState;

    DCHECK(request);
    request_ = request;

    auto* spec = request->spec();
    if (!spec || spec->stringified_method_data().count(kBat) == 0) {
      ChromePaymentRequestDelegate::ShowDialog(request);
      return;
    }

    double total_amount = 0;
    base::StringToDouble(spec->details().total->amount->value, &total_amount);

    controller_ = brave_rewards::ShowCheckoutDialog(web_contents_, {
      spec->details().total->label,
      total_amount,
      {} // TODO(zenparsing): Add sku/description items
    });

    controller_->SetOnDialogClosedCallback(base::BindOnce(
        &BravePaymentRequestDelegate::OnDialogClosed,
        weak_factory_.GetWeakPtr()));

    controller_->SetOnPaymentReadyCallback(base::BindOnce(
        &BravePaymentRequestDelegate::OnPaymentReady,
        weak_factory_.GetWeakPtr()));

    auto app = std::make_unique<BATPaymentApp>();
    payment_app_ = app->weak_factory_.GetWeakPtr();

    PaymentRequestState* state = request->state();
    state->OnPaymentAppCreated(std::move(app));

    DCHECK(!payment_app_.WasInvalidated());
    state->SetSelectedApp(
        payment_app_.get(),
        PaymentRequestState::SectionSelectionStatus::kAddedSelected);
  }

  void RetryDialog() override {
    // TODO(zenparsing): Do we need this?
  }

  void CloseDialog() override {
    if (controller_) {
      controller_->NotifyPaymentAborted();
    }
  }

  void ShowErrorMessage() override {
    // TODO(zenparsing): Do we need this?
  }

  void ShowProcessingSpinner() override {
    // TODO(zenparsing): Do we need this?
  }

 private:
  void OnDialogClosed(bool payment_fulfilled) {
    DCHECK(request_);
    request_->UserCancelled();
  }

  void OnPaymentReady() {
    DCHECK(request_);
    request_->Pay();
  }

  content::WebContents* web_contents_;
  payments::PaymentRequest* request_ = nullptr;
  base::WeakPtr<BATPaymentApp> payment_app_;
  base::WeakPtr<brave_rewards::CheckoutDialogController> controller_;
  base::WeakPtrFactory<BravePaymentRequestDelegate> weak_factory_;
};

}  // namespace

#include "../../../../../chrome/browser/payments/payment_request_factory.cc"
