//  http://code.google.com/p/android-color-picker/
/*
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


package com.graph89.controls;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.util.AttributeSet;
import android.util.FloatMath;
import android.view.View;

public class AmbilWarnaPrefWidgetView extends View {
	Paint paint;
	float rectSize;
	float strokeWidth;

	public AmbilWarnaPrefWidgetView(Context context, AttributeSet attrs) {
		super(context, attrs);
		
		float density = context.getResources().getDisplayMetrics().density;
		rectSize = FloatMath.floor(24.f * density + 0.5f);
		strokeWidth = FloatMath.floor(1.f * density + 0.5f);

		paint = new Paint();
		paint.setColor(0xffffffff);
		paint.setStyle(Style.STROKE);
		paint.setStrokeWidth(strokeWidth);
	}

	@Override protected void onDraw(Canvas canvas) {
		super.onDraw(canvas);

		canvas.drawRect(strokeWidth, strokeWidth, rectSize - strokeWidth, rectSize - strokeWidth, paint);
	}
}
