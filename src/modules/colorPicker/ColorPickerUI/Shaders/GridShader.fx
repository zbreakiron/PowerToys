float2 mousePosition : register(C1);
float radius : register(C2);
float squareSize : register(c3);
float textureSize : register(c4);

sampler2D inputSampler : register(S0);

float4 main(float2 uv : TEXCOORD) : COLOR
{
    // do not draw grid where the mouse is
    if (uv.x == mousePosition.y && uv.y == mousePosition.y)
    {
        return tex2D(inputSampler, uv);
    }

    float4 originalColor = tex2D(inputSampler, uv);
    float4 color = tex2D(inputSampler, uv);
    float distanceFromMouse = length(mousePosition - uv);
    float distanceFactor;

    int pixelPositionX = textureSize * uv.x;
    int pixelPositionY = textureSize * uv.y;

    int mousePositionX = mousePosition.x * textureSize;
    int mousePositionY = mousePosition.y * textureSize;

    if (distanceFromMouse <= radius)
    {
        // draw grid
        if (pixelPositionX % squareSize == 0 || pixelPositionY % squareSize == 0)
        {
            color.r = color.r + ((1.0 - color.r) * (1.0 - (distanceFromMouse / radius)));
            color.g = color.g + ((1.0 - color.g) * (1.0 - (distanceFromMouse / radius)));
            color.b = color.b + ((1.0 - color.b) * (1.0 - (distanceFromMouse / radius)));
        }
    }

    int2 topLeftRectangle = int2(mousePositionX - (mousePositionX % squareSize) - 1, mousePositionY - (mousePositionY % squareSize) - 1);

    if (((pixelPositionX >= topLeftRectangle.x && pixelPositionX <= topLeftRectangle.x + squareSize + 2) && (pixelPositionY == topLeftRectangle.y || pixelPositionY == topLeftRectangle.y + squareSize + 2)) ||
        ((pixelPositionY >= topLeftRectangle.y && pixelPositionY <= topLeftRectangle.y + squareSize + 2) && (pixelPositionX == topLeftRectangle.x || pixelPositionX == topLeftRectangle.x + squareSize + 2)))
    {
        originalColor.r = 1.0f;
        originalColor.g = 1.0f;
        originalColor.b = 1.0f;
        return originalColor;
    }

    return color;
}