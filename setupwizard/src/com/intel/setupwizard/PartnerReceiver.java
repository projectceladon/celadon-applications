/*
 * Copyright (C) 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.intel.car.setupwizard;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/**
 * This is not a real receiver, but only used as a marker interface so that Setup Wizard can resolve
 * this package and fetch resources from here.
 *
 * Partners should include a copy of this receiver in their package and in their manifest in order
 * to override custom resources in the setup wizard.
 */
public class PartnerReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
    }
}
