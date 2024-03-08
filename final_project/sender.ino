void setup() 
{
  Serial1.begin(115200);
}
void loop() 
{
  Serial1.write('x');
  delay(1000);
}
