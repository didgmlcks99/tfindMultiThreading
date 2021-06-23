class VehicleMain extends FinalVehicle { //Error
  private String modelName = "Mustang";
  public static void main(String[] args) {
    VehicleMain myFastCar = new VehicleMain();
    myFastCar.honk();
    System.out.println(myFastCar.brand + " " + myFastCar.modelName);
  }
}

