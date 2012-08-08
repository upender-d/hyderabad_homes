class Administrators::PropertiesController < ApplicationController

  def index
    if params[:search]
      @properties = Property.search_properties(params[:search]).paginate(:page => params[:page], :per_page => params[:per_page] , :order =>"created_at  DESC")
    else
      @properties = Property.paginate(:page => params[:page], :per_page => params[:per_page] , :order =>"created_at  DESC")
    end

  end

  def new
    @property = Property.new
  end

  def create
    @property = Property.new(params[:property])
    respond_to do |format|
      if  @property.save
        format.html { redirect_to ([:administrators,@property]) ,:notice => "Property Saved Successfully." }
      else
        format.html { render :action => "new" }
        format.xml  { render :xml => @property.errors, :status => :unprocessable_entity }
      end
    end
  end

  def show
    @property = Property.find(params[:id])
  end

  def edit
    @property = Property.find(params[:id])
  end

  def update
    @property = Property.find(params[:id])
    if  @property.update_attributes(params[:property])
      redirect_to([:administrators,@property])
      flash[:notice] = "Property Updated Successfully."
    else
      render :action => "edit"
    end
  end

  def destroy
    @property = Property.find(params[:id])
    @property.destroy
    redirect_to administrators_properties_path
  end

end
